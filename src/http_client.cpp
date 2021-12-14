#include "http_client.hpp"
#include "ilogger.hpp"
#include <string.h>
#include <unordered_map>

extern "C"{
	#include <curl/curl.h>
}

using namespace std;


HttpBodyData::HttpBodyData(const std::string& data){
    this->pdata = data.data();
    this->size = data.size();
}

HttpBodyData::HttpBodyData(const void* pdata, size_t size){
    this->pdata = pdata;
    this->size = size;
}

enum QueryType : int{
    QueryType_PostFrom,
    QueryType_PostBody,
    QueryType_PutBody,
    QueryType_PutFile,
    QueryType_Get,
    QueryType_Put
};

struct HttpClientReadStream{
    const unsigned char* pdata = nullptr;
    size_t data_size = 0;
    int curosr = 0;
};

class HttpClientImpl : public HttpClient{
public:
    HttpClientImpl(const string& url){

        curl_global_init(CURL_GLOBAL_ALL);

        url_ = url;
        add_header("Accept: */*");
        add_header("Charset: utf-8");
        //add_header("Expect:");
    }

    virtual const string& response_body() const override{
        return data_;
    }

    virtual int state_code() const override{
        return state_code_;
    }

    virtual HttpClient* verbose() override{
        verbose_ = true;
        return this;
    }

    virtual HttpClient* add_header(const string& value) override{
        headers_.emplace_back(value);
        return this;
    }

    virtual HttpClient* add_param(const string& name, const string& value) override{
        params_[name] = value;
        return this;
    }

    virtual HttpClient* timeout(int timeout_second) override {
        timeout_second_ = timeout_second;
        return this;
    }

    virtual bool post_body(const HttpBodyData& body) override{
        type_ = QueryType_PostBody;
        body_ = body;
        return query();
    }

    virtual bool put_body(const HttpBodyData& body) override{
        type_ = QueryType_PutBody;
        body_ = body;
        return query();
    }

    virtual bool put_file(const std::string& file) override{
        type_ = QueryType_PutFile;
        put_file_ = file;
        return query();
    }

    virtual bool put(){
        type_ = QueryType_Put;
        return query();
    }

    virtual bool post(){
        type_ = QueryType_PostFrom;
        return query();
    }

    virtual bool get(){
        type_ = QueryType_Get;
        return query();
    }

    static size_t write_bytes(void *ptr, size_t size, size_t count, void *stream){
        string* buffer = ((string*)stream);
        buffer->insert(buffer->end(), (char*)ptr, (char*)ptr + size* count);
        return size* count;
    }

    static size_t read_bytes(void *ptr, size_t size, size_t count, void *userdata){
        HttpClientReadStream* stream = ((HttpClientReadStream*)userdata);
        size_t remain = stream->data_size - stream->curosr;
        size_t copyed_size = min(size * count, remain);
        
        memcpy(ptr, stream->pdata + stream->curosr, copyed_size);
        stream->curosr += copyed_size;
        return copyed_size;
    }

    virtual const std::string response_header(const std::string& name) const override{

        auto iter = response_header_.find(name);
        if(iter == response_header_.end())
            return "";

        return iter->second;
    }

    virtual bool has_response_header(const std::string& name) const override{
        return response_header_.find(name) != response_header_.end();
    }

    virtual const std::vector<std::string>& response_headers() const override{
        return response_header_lines_;
    }

    virtual const std::string& error_message() const override{
        return error_;
    }

    bool query(){

        state_code_ = 0;
        data_.clear();
        response_header_lines_.clear();
        response_header_.clear();

        curl_slist* headers = nullptr;
        curl_httppost *formpost = nullptr;
        curl_httppost *lastptr = nullptr;
        CURL* curl = curl_easy_init();
        FILE* fput_file_handle = nullptr;
        size_t put_file_size = 0;
        HttpClientReadStream stream_put_body;

        if(type_ == QueryType_PutFile){

            put_file_size = iLogger::file_size(put_file_);
            fput_file_handle = fopen(put_file_.c_str(), "rb");
            if(fput_file_handle == nullptr){
                INFOE("Open file %s failed.", put_file_.c_str());
                return false;
            }
        }

        for (auto& k : params_){
            curl_formadd(&formpost,
                (curl_httppost**)&lastptr,
                CURLFORM_COPYNAME, k.first.c_str(),
                CURLFORM_COPYCONTENTS, k.second.c_str(),
                CURLFORM_END);
        }

        for(auto& value : headers_){
            headers = curl_slist_append(headers, value.c_str());
        }

         if(iLogger::begin_with(url_, "https://")){
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 1);
        }

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_URL, url_.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_bytes);
        curl_easy_setopt(curl, CURLOPT_HEADER, 1);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data_);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout_second_);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, verbose_ ? 1 : 0);

        if(type_ == QueryType_PostBody){
            curl_easy_setopt(curl, CURLOPT_POST, 1);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body_.pdata);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)body_.size);
        }else if(type_ == QueryType_PutBody){
            stream_put_body.pdata  = static_cast<const unsigned char*>(body_.pdata);
            stream_put_body.data_size = body_.size;
            stream_put_body.curosr = 0;
            curl_easy_setopt(curl, CURLOPT_PUT, 1);
            curl_easy_setopt(curl, CURLOPT_INFILE, &stream_put_body);
            curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_bytes);
            curl_easy_setopt(curl, CURLOPT_INFILESIZE, (long)body_.size);
        }else if(type_ == QueryType_PutFile){
            curl_easy_setopt(curl, CURLOPT_PUT, 1);
            curl_easy_setopt(curl, CURLOPT_INFILE, fput_file_handle);
            curl_easy_setopt(curl, CURLOPT_INFILESIZE, (long)put_file_size);
        }else if(type_ == QueryType_PostFrom){
            curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
        }else if(type_ == QueryType_Put){
            curl_easy_setopt(curl, CURLOPT_PUT, 1);
            curl_easy_setopt(curl, CURLOPT_INFILE, nullptr);
            curl_easy_setopt(curl, CURLOPT_INFILESIZE, (long)0);
        }

        bool ok = false;
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            error_ = iLogger::format("Curl error, code is %d, %s", res, curl_easy_strerror(res));
            goto err;
        } 

        // do curl query
        {
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &state_code_);
            if (!(state_code_ >= 200 && state_code_ < 300)){
                error_ = iLogger::format("Response code: %d\n", state_code_);
                goto err;
            }

            int line_position = data_.find("\r\n\r\n");
            if(line_position == -1){
                error_ = iLogger::format("Worng http response, no line token, code: %d\n", state_code_);
                goto err;
            }

            response_header_string_ = data_.substr(0, line_position);
            data_ = data_.substr(line_position + 4);
            
            response_header_lines_ = iLogger::split_string(response_header_string_, "\r\n");
            for(auto& line : response_header_lines_){
                int token_position = line.find(':');
                if(token_position == -1) 
                    continue;

                auto key = line.substr(0, token_position);
                auto value = line.substr(token_position + 2);
                response_header_[key] = value;
            }
            ok = true;
        }

    err:
        curl_easy_cleanup(curl);
        curl_formfree(formpost);
        curl_slist_free_all(headers);

        if(fput_file_handle != nullptr){
            fclose(fput_file_handle);
        }
        return ok;
    }

    virtual const std::string& response_header_string() const {
        return response_header_string_;
    }

private:
    string url_;
    unordered_map<string, string> params_;
    vector<string> headers_;
    string response_header_string_;
    unordered_map<string, string> response_header_;
    vector<string> response_header_lines_;
    string data_;
    string error_;
    QueryType type_;
    HttpBodyData body_;
    string put_file_;
    int state_code_ = 0;
    bool verbose_ = false;
    int timeout_second_ = 60;
};

shared_ptr<HttpClient> newHttp(const string& url){
    return shared_ptr<HttpClientImpl>(new HttpClientImpl(url));
}