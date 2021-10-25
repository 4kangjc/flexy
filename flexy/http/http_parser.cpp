#include "http_parser.h"
#include "flexy/util/config.h"

namespace flexy::http {

static auto g_logger = FLEXY_LOG_NAME("system");

static auto g_http_request_buffer_size = Config::Lookup("http.request.buffer_size", 
    (uint64_t)4 * 1024, "http request buffer size");
static auto g_http_request_max_body_size = Config::Lookup("http.request.max_bopdy_size", 
    (uint64_t)64 * 1024, "http request max body size");
static auto g_http_response_buffer_size = Config::Lookup("http.response.buffer_size", 
    (uint64_t)4 * 1024, "http response buffer size");
static auto g_http_response_max_body_size = Config::Lookup("http.response.max_bopdy_size", 
    (uint64_t)64 * 1024, "http response max body size");

static uint64_t s_http_request_buffer_size = 0;
static uint64_t s_http_request_max_body_size = 0;
static uint64_t s_http_response_buffer_size = 0;
static uint64_t s_http_response_max_body_size = 0;

uint64_t HttpRequestParser::GetHttpRequestBufferSize() {
    return s_http_request_buffer_size;
}

uint64_t HttpRequestParser::GetHttpRequestMaxBodySize() {
    return s_http_request_max_body_size;
}

uint64_t HttpResponseParser::GetHttpResponseBufferSize() {
    return s_http_response_buffer_size;
}

uint64_t HttpResponseParser::GetHttpResponseMaxBodySize() {
    return s_http_response_max_body_size;
}

namespace {

struct _RequestSizeIniter {
    _RequestSizeIniter() {
        s_http_request_buffer_size = g_http_request_buffer_size->getValue();
        s_http_request_max_body_size = g_http_request_max_body_size->getValue();
        s_http_response_buffer_size = g_http_response_buffer_size->getValue();
        s_http_response_max_body_size = g_http_response_max_body_size->getValue();
        g_http_request_buffer_size->addListener([](const uint64_t& ov, uint64_t nv) {
            s_http_request_buffer_size = nv;
        });
        g_http_request_max_body_size->addListener([](const uint64_t& ov, uint64_t nv) {
            s_http_request_max_body_size = nv;
        });
        g_http_response_buffer_size->addListener([](const uint64_t& ov, uint64_t nv) {
            s_http_response_buffer_size = nv;
        });
        g_http_response_max_body_size->addListener([](const uint64_t& ov, uint64_t nv) {
            s_http_response_max_body_size = nv;
        });
    }
};
static _RequestSizeIniter _init;

} // namespace

void on_request_method(void* data, const char* at, size_t length) {
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    HttpMethod m = StringToHttpMethod(std::string_view(at, length));
    if (m == HttpMethod::INVALID_METHOD) {
        FLEXY_LOG_ERROR(g_logger) << "invalid http request method: "
        << std::string_view(at, length);
        parser->setError(1000);
        return;
    }
    parser->getData()->setMethod(m);
}

void on_request_uri(void *data, const char *at, size_t length) {

}

void on_requese_fragment(void *data, const char *at, size_t length) {
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    parser->getData()->setFragment(std::string_view(at, length));
}

void on_request_path(void *data, const char *at, size_t length) {
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    parser->getData()->setPath(std::string_view(at, length));
}

void on_request_query(void *data, const char *at, size_t length) {
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    parser->getData()->setQuery(std::string_view(at, length));
}

void on_request_version(void *data, const char *at, size_t length) {
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    uint8_t v = 0;
    if (strncmp(at, "HTTP/1.1", length) == 0) {
        v = 0x01;
    } else if (strncmp(at, "HTTP/1.0", length) == 0) {
        v = 0x10;
    } else {
        FLEXY_LOG_WARN(g_logger) << "invalid http request version: " 
        << std::string_view(at, length);
        parser->setError(1001);
        return;
    }
    parser->getData()->setVersion(v);
}

void on_request_header_done(void *data, const char *at, size_t length) {

}

void on_request_http_field(void *data, const char *field, size_t flen, const char *value, size_t vlen) {
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    if (flen == 0) {
        FLEXY_LOG_WARN(g_logger) << "invalid http request field length = 0";
        return; 
    }
    parser->getData()->setHeader(std::string(field, flen), std::string(value, vlen));
}

HttpRequestParser::HttpRequestParser() : data_(new HttpRequest), error_(0) {
    http_parser_init(&parser_);
    parser_.request_method = on_request_method;
    parser_.request_uri = on_request_uri;
    parser_.fragment = on_requese_fragment;
    parser_.request_path = on_request_path;
    parser_.query_string = on_request_query;
    parser_.http_version = on_request_version;
    parser_.header_done = on_request_header_done;
    parser_.http_field = on_request_http_field;
    parser_.data = this;
}

uint64_t HttpRequestParser::getContentLength() const {
    return data_->getHeaderAs<uint64_t>("content-length", 0);
}

size_t HttpRequestParser::execute(char* data, size_t len) {
    size_t offset = http_parser_execute(&parser_, data, len, 0);
    memmove(data, data + offset, len - offset);
    return offset;
}

int HttpRequestParser::isFinished() {
    return http_parser_finish(&parser_);
}

int HttpRequestParser::hasError() {
    return error_ && http_parser_has_error(&parser_);
}

void on_response_reason(void *data, const char *at, size_t length) {
    HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
    parser->getData()->setReason(std::string_view(at, length));
}

void on_response_status(void *data, const char *at, size_t length) {
    HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
    HttpStatus status = (HttpStatus)(atoi(at, at + length));
    parser->getData()->setStatus(status);
}

void on_response_chunk(void *data, const char *at, size_t length) {

}

void on_response_version(void *data, const char *at, size_t length) {
    HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
    uint8_t v = 0;
    if (strncmp(at, "HTTP/1.1", length) == 0) {
        v = 0x11;
    } else if (strncmp(at, "HTTP/1.0", length) == 0) {
        v = 0x10;
    } else {
        FLEXY_LOG_WARN(g_logger) << "invalid http response version: "
        << std::string_view(at, length);
        parser->setError(1001);
        return;
    }
    parser->getData()->setVersion(v);
}

void on_response_header_done(void *data, const char *at, size_t length) {

}

void on_response_last_chunk(void *data, const char *at, size_t length) {

}

void on_response_http_field(void *data, const char *field, size_t flen, const char *value, size_t vlen) {
    HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
    if (flen == 0) {
        FLEXY_LOG_WARN(g_logger) << "invalid http response field length = 0";
        return;
    }
    parser->getData()->setHeader(std::string(field, flen), std::string(value, vlen));
}

HttpResponseParser::HttpResponseParser() : data_(new HttpResponse), error_(0) {
    httpclient_parser_init(&parser_);
    parser_.reason_phrase = on_response_reason;
    parser_.status_code = on_response_status;
    parser_.chunk_size = on_response_chunk;
    parser_.http_version = on_response_version;
    parser_.header_done = on_response_header_done;
    parser_.last_chunk = on_response_last_chunk;
    parser_.http_field = on_response_http_field;
    parser_.data = this;
}

size_t HttpResponseParser::execute(char* data, size_t len, bool chunck) {
    if (chunck) {
        httpclient_parser_init(&parser_);
    }
    size_t offset = httpclient_parser_execute(&parser_, data, len, 0);
    memmove(data, data + offset, (len - offset));
    return offset;
}

int HttpResponseParser::isFinished() {
    return httpclient_parser_finish(&parser_);
}

int HttpResponseParser::hasError() {
    return error_ && httpclient_parser_has_error(&parser_);
}

uint64_t HttpResponseParser::getContentLength() const {
    return data_->getHeaderAs<uint64_t>("content-length", 0);
}

} // namespace flexy http