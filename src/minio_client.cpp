#include "minio_client.hpp"
#include <string.h>
#include <openssl/hmac.h>

#include "http_client.hpp"
#include "ilogger.hpp"

using namespace std;

static string base64_encode(const void* data, size_t size) {

    string encode_result;
    encode_result.reserve(size / 3 * 4 + (size % 3 != 0 ? 4 : 0));

    const unsigned char * current = static_cast<const unsigned char*>(data);
    static const char *base64_table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";  
    while(size > 2) {
        encode_result += base64_table[current[0] >> 2];
        encode_result += base64_table[((current[0] & 0x03) << 4) + (current[1] >> 4)];
        encode_result += base64_table[((current[1] & 0x0f) << 2) + (current[2] >> 6)];
        encode_result += base64_table[current[2] & 0x3f];

        current += 3;
        size -= 3;
    }

    if(size > 0){
        encode_result += base64_table[current[0] >> 2];
        if(size%3 == 1) {
            encode_result += base64_table[(current[0] & 0x03) << 4];
            encode_result += "==";
        } else if(size%3 == 2) {
            encode_result += base64_table[((current[0] & 0x03) << 4) + (current[1] >> 4)];
            encode_result += base64_table[(current[1] & 0x0f) << 2];
            encode_result += "=";
        }
    }
    return encode_result;
}

// 与date -R 结果一致
static string gmtime_now(){
    time_t timet;
    time(&timet);

    tm& t = *(tm*)localtime(&timet);
    char timebuffer[100];
    strftime(timebuffer, sizeof(timebuffer), "%a, %d %b %Y %X %z", &t);
    return timebuffer;
}

// openssl sha1 -hmac -binary
static string hmac_encode_base64(const string& key, const void* data, size_t size){

    // SHA1 needed 20 characters.
    unsigned int len = 20;
    unsigned char result[20];
 
    HMAC_CTX* ctx = HMAC_CTX_new();
    HMAC_Init_ex(ctx, key.data(), key.size(), EVP_sha1(), NULL);
    HMAC_Update(ctx, (unsigned char*)data, size);
    HMAC_Final(ctx, result, &len);
    HMAC_CTX_free(ctx);
    return base64_encode(result, len);
}

// echo -en ${_signature} | openssl sha1 -hmac ${s3_secret} -binary | base64
static string minio_hmac_encode(
    const string& hash_key,
    const string& method,
    const string& content_type, 
    const string& time,
    const string& path
){
    char buffer[1000];
    int result_length = snprintf(
        buffer, sizeof(buffer), 
        "%s\n\n%s\n%s\n%s", 
        method.c_str(),
        content_type.c_str(), 
        time.c_str(), 
        path.c_str()
    );
    return hmac_encode_base64(hash_key, buffer, result_length);
}

static string extract_name(const string& response, int begin, int end){
    
    int p = response.find("<Name>", begin);
    if(p == -1 || p >= end) return "";
    p += 6;

    int e = response.find("</Name>", p);
    if(e == -1 || p >= e) return "";
    return string(response.begin() + p, response.begin() + e);
}

static vector<string> extract_buckets(const string& response){

    string bucket_b_token = "<Bucket>";
    string bucket_e_token = "</Bucket>";
    vector<string> names;
    int p = response.find(bucket_b_token);
    while(p != -1){
        int e = response.find(bucket_e_token, p + bucket_b_token.size());
        if(e == -1)
            break;

        names.emplace_back(move(extract_name(response, p, e)));
        p = response.find(bucket_b_token, e + bucket_e_token.size());
    }
    return names;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

MinioClient::MinioClient(const string& server, const string& access_key, const string& secret_key)
:server(server), access_key(access_key), secret_key(secret_key)
{}

bool MinioClient::upload_file(
    const string& remote_path,
    const string& file
){
    const char* content_type = "application/octet-stream";
    auto time = gmtime_now();
    auto signature = minio_hmac_encode(
        secret_key, "PUT", content_type, time, remote_path
    );

    auto http = newHttp(iLogger::format("%s%s", server.c_str(), remote_path.c_str()));
    bool success = http
        ->add_header(iLogger::format("Date: %s", time.c_str()))
        ->add_header(iLogger::format("Content-Type: %s", content_type))
        ->add_header(iLogger::format("Authorization: AWS %s:%s", access_key.c_str(), signature.c_str()))
        ->put_file(file);

    if(!success){
        INFOE("post failed: %s\n%s", http->error_message().c_str(), http->response_body().c_str());
    }
    return success;
}

bool MinioClient::upload_filedata(
    const string& remote_path,
    const string& filedata
){
    return upload_filedata(remote_path, filedata.data(), filedata.size());
}

bool MinioClient::upload_filedata(
    const string& remote_path,
    const void* file_data, size_t data_size
){
    const char* content_type = "application/octet-stream";
    auto time = gmtime_now();
    auto signature = minio_hmac_encode(
        secret_key, "PUT", content_type, time, remote_path
    );

    auto http = newHttp(iLogger::format("%s%s", server.c_str(), remote_path.c_str()));
    bool success = http
        ->add_header(iLogger::format("Date: %s", time.c_str()))
        ->add_header(iLogger::format("Content-Type: %s", content_type))
        ->add_header(iLogger::format("Authorization: AWS %s:%s", access_key.c_str(), signature.c_str()))
        ->put_body(HttpBodyData(file_data, data_size));

    if(!success){
        INFOE("post failed: %s\n%s", http->error_message().c_str(), http->response_body().c_str());
    }
    return success;
}

bool MinioClient::make_bucket(const std::string& name){

    string path = "/" + name;
    const char* content_type = "text/plane";

    auto time = gmtime_now();
    auto signature = minio_hmac_encode(
        secret_key, "PUT", content_type, time, path
    );

    auto http = newHttp(iLogger::format("%s%s", server.c_str(), path.c_str()));
    bool success = http
        ->add_header(iLogger::format("Date: %s", time.c_str()))
        ->add_header(iLogger::format("Content-Type: %s", content_type))
        ->add_header(iLogger::format("Authorization: AWS %s:%s", access_key.c_str(), signature.c_str()))
        ->put();

    if(!success){
        INFOE("post failed: %s\n%s", http->error_message().c_str(), http->response_body().c_str());
    }
    return success;
}

vector<string> MinioClient::get_bucket_list(bool* pointer_success){
    const char* path = "/";
    const char* content_type = "text/plane";

    auto time = gmtime_now();
    auto signature = minio_hmac_encode(
        secret_key, "GET", content_type, time, path
    );

    auto http = newHttp(iLogger::format("%s%s", server.c_str(), path));
    bool success = http
        ->add_header(iLogger::format("Date: %s", time.c_str()))
        ->add_header(iLogger::format("Content-Type: %s", content_type))
        ->add_header(iLogger::format("Authorization: AWS %s:%s", access_key.c_str(), signature.c_str()))
        ->get();

    if(pointer_success)
        *pointer_success = success;

    if(!success){
        INFOE("post failed: %s\n%s", http->error_message().c_str(), http->response_body().c_str());
        return {};
    }
    return extract_buckets(http->response_body());
}

string MinioClient::get_file(
    const string& remote_path, bool* pointer_success
){
    const char* content_type = "application/octet-stream";
    auto time = gmtime_now();
    auto signature = minio_hmac_encode(
        secret_key, "GET", content_type, time, remote_path
    );

    auto http = newHttp(iLogger::format("%s%s", server.c_str(), remote_path.c_str()));
    bool success = http
        ->add_header(iLogger::format("Date: %s", time.c_str()))
        ->add_header(iLogger::format("Content-Type: %s", content_type))
        ->add_header(iLogger::format("Authorization: AWS %s:%s", access_key.c_str(), signature.c_str()))
        ->get();

    if(pointer_success)
        *pointer_success = success;

    if(!success){
        INFOE("post failed: %s\n%s", http->error_message().c_str(), http->response_body().c_str());
        return "";
    }
    return http->response_body();
}