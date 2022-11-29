#pragma once
#include <map>
#include <sstream>
#include <cstring>
#include <memory>
#include <boost/lexical_cast.hpp>

namespace flexy::http {

// TODO std::optional
template <typename MapType, typename T>
bool checkGetAs(const MapType& m, const std::string& key, T& val, const T& def = T()) {
    std::string str;
    auto it = m.find(key);
    if (it == m.end()) {
        val = def;
        return false;
    }
    try {
        val = boost::lexical_cast<T>(it->second);
        return true;
    } catch (...) {
        val = def;
    }
    return false;
}

template <typename MapType, typename T>
T getAs(const MapType& m, const std::string& key, const T& def = T()) {
    auto it = m.find(key);
    if (it == m.end()){
        return def;
    }
    try {
        return boost::lexical_cast<T>(it->second);
    } catch (...) {

    }
    return def;
}
    /* Request Methods */
#define HTTP_METHOD_MAP(XX)         \
  XX(0,  DELETE,      DELETE)       \
  XX(1,  GET,         GET)          \
  XX(2,  HEAD,        HEAD)         \
  XX(3,  POST,        POST)         \
  XX(4,  PUT,         PUT)          \
  /* pathological */                \
  XX(5,  CONNECT,     CONNECT)      \
  XX(6,  OPTIONS,     OPTIONS)      \
  XX(7,  TRACE,       TRACE)        \
  /* WebDAV */                      \
  XX(8,  COPY,        COPY)         \
  XX(9,  LOCK,        LOCK)         \
  XX(10, MKCOL,       MKCOL)        \
  XX(11, MOVE,        MOVE)         \
  XX(12, PROPFIND,    PROPFIND)     \
  XX(13, PROPPATCH,   PROPPATCH)    \
  XX(14, SEARCH,      SEARCH)       \
  XX(15, UNLOCK,      UNLOCK)       \
  XX(16, BIND,        BIND)         \
  XX(17, REBIND,      REBIND)       \
  XX(18, UNBIND,      UNBIND)       \
  XX(19, ACL,         ACL)          \
  /* subversion */                  \
  XX(20, REPORT,      REPORT)       \
  XX(21, MKACTIVITY,  MKACTIVITY)   \
  XX(22, CHECKOUT,    CHECKOUT)     \
  XX(23, MERGE,       MERGE)        \
  /* upnp */                        \
  XX(24, MSEARCH,     M-SEARCH)     \
  XX(25, NOTIFY,      NOTIFY)       \
  XX(26, SUBSCRIBE,   SUBSCRIBE)    \
  XX(27, UNSUBSCRIBE, UNSUBSCRIBE)  \
  /* RFC-5789 */                    \
  XX(28, PATCH,       PATCH)        \
  XX(29, PURGE,       PURGE)        \
  /* CalDAV */                      \
  XX(30, MKCALENDAR,  MKCALENDAR)   \
  /* RFC-2068, section 19.6.1.2 */  \
  XX(31, LINK,        LINK)         \
  XX(32, UNLINK,      UNLINK)       \
  /* icecast */                     \
  XX(33, SOURCE,      SOURCE)       \


  /* Status Codes */
#define HTTP_STATUS_MAP(XX)                                                 \
  XX(100, CONTINUE,                        Continue)                        \
  XX(101, SWITCHING_PROTOCOLS,             Switching Protocols)             \
  XX(102, PROCESSING,                      Processing)                      \
  XX(200, OK,                              OK)                              \
  XX(201, CREATED,                         Created)                         \
  XX(202, ACCEPTED,                        Accepted)                        \
  XX(203, NON_AUTHORITATIVE_INFORMATION,   Non-Authoritative Information)   \
  XX(204, NO_CONTENT,                      No Content)                      \
  XX(205, RESET_CONTENT,                   Reset Content)                   \
  XX(206, PARTIAL_CONTENT,                 Partial Content)                 \
  XX(207, MULTI_STATUS,                    Multi-Status)                    \
  XX(208, ALREADY_REPORTED,                Already Reported)                \
  XX(226, IM_USED,                         IM Used)                         \
  XX(300, MULTIPLE_CHOICES,                Multiple Choices)                \
  XX(301, MOVED_PERMANENTLY,               Moved Permanently)               \
  XX(302, FOUND,                           Found)                           \
  XX(303, SEE_OTHER,                       See Other)                       \
  XX(304, NOT_MODIFIED,                    Not Modified)                    \
  XX(305, USE_PROXY,                       Use Proxy)                       \
  XX(307, TEMPORARY_REDIRECT,              Temporary Redirect)              \
  XX(308, PERMANENT_REDIRECT,              Permanent Redirect)              \
  XX(400, BAD_REQUEST,                     Bad Request)                     \
  XX(401, UNAUTHORIZED,                    Unauthorized)                    \
  XX(402, PAYMENT_REQUIRED,                Payment Required)                \
  XX(403, FORBIDDEN,                       Forbidden)                       \
  XX(404, NOT_FOUND,                       Not Found)                       \
  XX(405, METHOD_NOT_ALLOWED,              Method Not Allowed)              \
  XX(406, NOT_ACCEPTABLE,                  Not Acceptable)                  \
  XX(407, PROXY_AUTHENTICATION_REQUIRED,   Proxy Authentication Required)   \
  XX(408, REQUEST_TIMEOUT,                 Request Timeout)                 \
  XX(409, CONFLICT,                        Conflict)                        \
  XX(410, GONE,                            Gone)                            \
  XX(411, LENGTH_REQUIRED,                 Length Required)                 \
  XX(412, PRECONDITION_FAILED,             Precondition Failed)             \
  XX(413, PAYLOAD_TOO_LARGE,               Payload Too Large)               \
  XX(414, URI_TOO_LONG,                    URI Too Long)                    \
  XX(415, UNSUPPORTED_MEDIA_TYPE,          Unsupported Media Type)          \
  XX(416, RANGE_NOT_SATISFIABLE,           Range Not Satisfiable)           \
  XX(417, EXPECTATION_FAILED,              Expectation Failed)              \
  XX(421, MISDIRECTED_REQUEST,             Misdirected Request)             \
  XX(422, UNPROCESSABLE_ENTITY,            Unprocessable Entity)            \
  XX(423, LOCKED,                          Locked)                          \
  XX(424, FAILED_DEPENDENCY,               Failed Dependency)               \
  XX(426, UPGRADE_REQUIRED,                Upgrade Required)                \
  XX(428, PRECONDITION_REQUIRED,           Precondition Required)           \
  XX(429, TOO_MANY_REQUESTS,               Too Many Requests)               \
  XX(431, REQUEST_HEADER_FIELDS_TOO_LARGE, Request Header Fields Too Large) \
  XX(451, UNAVAILABLE_FOR_LEGAL_REASONS,   Unavailable For Legal Reasons)   \
  XX(500, INTERNAL_SERVER_ERROR,           Internal Server Error)           \
  XX(501, NOT_IMPLEMENTED,                 Not Implemented)                 \
  XX(502, BAD_GATEWAY,                     Bad Gateway)                     \
  XX(503, SERVICE_UNAVAILABLE,             Service Unavailable)             \
  XX(504, GATEWAY_TIMEOUT,                 Gateway Timeout)                 \
  XX(505, HTTP_VERSION_NOT_SUPPORTED,      HTTP Version Not Supported)      \
  XX(506, VARIANT_ALSO_NEGOTIATES,         Variant Also Negotiates)         \
  XX(507, INSUFFICIENT_STORAGE,            Insufficient Storage)            \
  XX(508, LOOP_DETECTED,                   Loop Detected)                   \
  XX(510, NOT_EXTENDED,                    Not Extended)                    \
  XX(511, NETWORK_AUTHENTICATION_REQUIRED, Network Authentication Required) \

enum class HttpMethod {
#define XX(num, name, string) name = num, 
    HTTP_METHOD_MAP(XX)
#undef XX
    INVALID_METHOD
};

enum class HttpStatus {
#define XX(code, name, desc) name = code, 
    HTTP_STATUS_MAP(XX)
#undef XX
};

HttpMethod StringToHttpMethod(std::string_view m);
const char* HttpMethodToString(HttpMethod m);
const char* HttpStatusToString(HttpStatus s);

struct CaseInsensitiveLess {
    bool operator()(const std::string& lhs, const std::string& rhs) const {
        return strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
    }
};

// Http 响应报文
class HttpResponse {
public:
    using ptr = std::unique_ptr<HttpResponse>;
    using MapType = std::map<std::string, std::string, CaseInsensitiveLess>;
    HttpResponse(uint8_t version = 0x11, bool close = true);
    HttpStatus getStatus() const { return status_; }
    uint8_t getVersion() const { return version_; }
    auto& getBody() const { return body_; }
    auto& getReason() const { return reason_; }
    auto& getHeaders() const { return headers_; }

    void setStatus(HttpStatus status) { status_ = status; }
    void setVersion(uint8_t version) { version_ = version; }
    void setBody(std::string_view body) { body_ = body; }
    void setReason(std::string_view reason) { reason_ = reason; }
    void setHeaders(const MapType& v) { headers_ = v; }

    bool isClose() const { return close_; }
    void setClose(bool v) { close_ = v; }

    std::string getHeader(const std::string& key, const std::string& def = "") const;
    void setHeader(const std::string& key, const std::string& val);
    void delHeader(const std::string& key);

    template <typename T>
    bool checkGetHeaderAs(const std::string& key, T& val, const T& def = T()) {
        return checkGetAs(headers_, key, val, def);
    }

    template <typename T>
    T getHeaderAs(const std::string& key, const T& def = T()) const {
        return getAs(headers_, key, def);
    }

    std::ostream& dump(std::ostream& os) const;
    std::string toString() const {
        std::stringstream ss;
        dump(ss);
        return ss.str();
    }
private:
    HttpStatus status_;                     // 响应状态码
    uint8_t version_;                       // 版本
    bool close_;                            // 是否自动关闭

    std::string body_;                      // 响应消息体
    std::string reason_;                    // 响应原因
    mutable MapType headers_;               // 响应头部报文
};

// Htpp请求报文
class HttpRequest {
public:
    using ptr = std::unique_ptr<HttpRequest>;
    using MapType = std::map<std::string, std::string, CaseInsensitiveLess>;

    HttpRequest(uint8_t version = 0x11, bool close = true);

    std::unique_ptr<HttpResponse> createResponse() { 
        return std::make_unique<HttpResponse>(version_, close_); 
    }

    HttpMethod getMehod() const { return method_; }
    uint8_t getVersion() const { return version_; }
    auto& getPath() const { return path_; }
    auto& getQuery() const { return query_; }
    auto& getBody() const { return body_; }
    std::string getUri();
    uint32_t getStreamId() { return streamId_; }

    auto& getHeaders() const { return headers_; }
    auto& getParams() const { return params_; }
    auto& getCookies() const { return cookies_; }

    void setMethod(HttpMethod v) { method_ = v; }
    void setVersion(uint8_t v) { version_ = v; }

    void setPath(std::string_view v) { path_ = v; }
    void setQuery(std::string_view v) { query_ = v; }
    void setFragment(std::string_view v) { fragment_ = v; }
    void setBody(std::string_view v) { body_ = v; }
    void setUri(std::string_view v);
    void setStreamId(uint32_t v) { streamId_ = v; }

    bool isClose() const { return close_; }
    void setClose(bool v) { close_ = v; } 

    void setHeaders(const MapType& v) { headers_ = v; }
    void setParams(const MapType& v) { params_ = v; }
    void setCookies(const MapType& v) { cookies_ = v; }

    std::string getHeader(const std::string& key, const std::string& def = "") const;
    std::string getParam(const std::string& key, const std::string& def = "") const;
    std::string getCookie(const std::string& key, const std::string& def = "") const;

    void setHeader(const std::string& key, const std::string& val);
    void setParam (const std::string& key, const std::string& val);
    void setCookie(const std::string& key, const std::string& val);

    void delHeader(const std::string& key);
    void delParam (const std::string& key);
    void delCookie(const std::string& key);

    bool hasHeader(const std::string& key, std::string* val = nullptr);
    bool hasParam (const std::string& key, std::string* val = nullptr);
    bool hasCookie(const std::string& key, std::string* val = nullptr);

    template <typename T>
    std::pair<typename MapType::iterator, bool> 
    try_emplaceHeader(const std::string& key, T&& val) {
        return headers_.try_emplace(key, std::forward<T>(val));
    }

    template <typename T>
    bool checkGetHeaderAs(const std::string& key, T& val, const T& def = T()) {
        return checkGetAs(headers_, key, val, def);
    }

    template <typename T>
    T getHeaderAs(const std::string& key, const T& def = T()) const {
        return getAs(headers_, key, def);
    }

    template <typename T>
    bool checkGetParamAs(const std::string& key, T& val, const T& def = T()) {
        return checkAs(params_, key, val, def);
    }

    template <typename T>
    T getParamAs(const std::string& key, const T& def = T()) const {
        return getAs(params_, key, def);
    }

    template <typename T>
    bool checkGetCookieAs(const std::string& key, T& val, const T& def = T()) {
        return checkAs(cookies_, key, val, def);
    }

    template <typename T>
    T getCookieAs(const std::string& key, const T& def = T()) const {
        return getAs(cookies_, key, def);
    }

    std::ostream& dump(std::ostream& os) const;
    std::string toString() const {
        std::stringstream ss;
        dump(ss);
        return ss.str();
    }
    // 更新connection： close or keep-alive
    void init();
private:
    HttpMethod method_;                     // Http方法
    uint8_t version_;                       // Http 版本
    bool close_;                            // 是否自动关闭

    uint8_t parseParmFlag_;                 // 

    uint32_t streamId_ = 0;                 // http2 流id

    std::string path_;                      // 请求路径
    std::string query_;                     // 请求参数
    std::string fragment_;                  // 请求fragment
    std::string body_;                      // 请求消息体

    mutable MapType headers_;               // 请求头部 Map
    MapType params_;                        // 请求参数 Map
    MapType cookies_;                       // 请求 Cookie Map
};

inline std::ostream& operator<<(std::ostream& os, const HttpRequest& req) {
    return req.dump(os);
}

inline std::ostream& operator<<(std::ostream& os, const HttpResponse& rsp) {
    return rsp.dump(os);
}

} // namespace flexy http
