#ifndef HTTP_CLIENT_HPP
#define HTTP_CLIENT_HPP

#include <string>
#include <vector>
#include <memory>

class HttpClient{
public:
    virtual HttpClient* add_header(const std::string& value) = 0;
    virtual HttpClient* add_param(const std::string& name, const std::string& value) = 0;
    virtual HttpClient* verbose() = 0;
    virtual HttpClient* timeout(int timeout_second) = 0;
    virtual bool post() = 0;
    virtual bool post_body(const std::string& body) = 0;
    virtual bool put_body(const std::string& body) = 0;
    virtual bool put_file(const std::string& file) = 0;
    virtual bool get() = 0;
    virtual bool put() = 0;
    virtual const std::string& response_body() const = 0;
    virtual int state_code() const = 0;
    virtual const std::string& error_message() const = 0;
    virtual const std::string response_header(const std::string& name) const = 0;
    virtual const std::vector<std::string>& response_headers() const = 0;
    virtual bool has_response_header(const std::string& name) const = 0;
    virtual const std::string& response_header_string() const = 0;
};

std::shared_ptr<HttpClient> newHttp(const std::string& url);

#endif //HTTP_CLIENT_HPP
