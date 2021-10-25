#pragma once

#include "http.h"
#include "http11_parser.h"
#include "httpclient_parser.h"

namespace flexy::http {

class HttpRequestParser {
public:
    HttpRequestParser(); 

    size_t execute(char* data, size_t len);
    int isFinished();
    int hasError();
    void setError(int v) { error_ = v; }
    auto& getData() { return data_; }
    uint64_t getContentLength() const;
    auto& getParser() const { return parser_; }

public:
    static uint64_t GetHttpRequestBufferSize();
    static uint64_t GetHttpRequestMaxBodySize();
private:
    http_parser parser_;
    HttpRequest::ptr data_;
    int error_; /*
 * 1000 : invalid method
 * 1001 : invalid version
 * 1002 : invalid field
 */
};

class HttpResponseParser {
public:
    HttpResponseParser();
    size_t execute(char* data, size_t len, bool chunck);
    int isFinished();
    int hasError();
    void setError(int v) { error_ = v; }
    auto& getData() { return data_; }
    uint64_t getContentLength() const;

    auto& getParser() const { return parser_; }

    static uint64_t GetHttpResponseBufferSize();
    static uint64_t GetHttpResponseMaxBodySize();
private:
    httpclient_parser parser_;
    HttpResponse::ptr data_;
    int error_;
};

} // namespace flexy http