#include "http.h"

namespace flexy::http {

static std::unordered_map<std::string_view, HttpMethod> s_method_name = {
#define XX(num, name, string) { #name, HttpMethod::name }, 
    HTTP_METHOD_MAP(XX)
#undef XX
};

HttpMethod StringToHttpMethod(std::string_view m) {
    auto it = s_method_name.find(m);
    return it == s_method_name.end() ? HttpMethod::INVALID_METHOD : it->second;
}

static const char* s_method_string[] = {
#define XX(num, name, string) #string, 
    HTTP_METHOD_MAP(XX)
#undef XX
};

const char* HttpMethodToString(HttpMethod m) {
    uint32_t idx = (uint32_t)m;
    if (idx >= 34) {
        return "<unknown>";
    }
    return s_method_string[idx];
}

const char* HttpStatusToString(HttpStatus s) {
    switch (s) {
#define XX(code, name, msg)    \
        case HttpStatus::name: \
            return #msg;
        HTTP_STATUS_MAP(XX);
#undef XX
        default:
            return "<unkown>";
    }
}

HttpRequest::HttpRequest(uint8_t version, bool close) :
    method_(HttpMethod::GET), version_(version), close_(false),
    parseParmFlag_(0), path_("/") {

}

std::string HttpRequest::getUri() {
    return path_ + (query_.empty() ? "" : "?" + query_) +
           (fragment_.empty() ? "" : "#" + fragment_);
}

std::string HttpRequest::getHeader(const std::string& key,
                                          const std::string& def) const {
    auto it = headers_.find(key);
    return it == headers_.end() ? def : it->second;
}

std::string HttpRequest::getParam(const std::string& key,
                                         const std::string& def) const {
    auto it = params_.find(key);
    return it == params_.end() ? def : it->second;
}

std::string HttpRequest::getCookie(const std::string& key,
                                          const std::string& def) const {
    auto it = cookies_.find(key);
    return it == cookies_.end() ? def : it->second;
}

void HttpRequest::setUri(std::string_view uri) {
    auto pos = uri.find('?');
    if (pos == std::string::npos) {
        auto pos2 = uri.find('#');
        if (pos2 == std::string_view::npos) {
            path_ = uri;
        } else {
            path_ = uri.substr(0, pos2);
            fragment_ = uri.substr(pos2 + 1);
        }
    } else {
        path_ = uri.substr(0, pos);

        auto pos2 = uri.find('#', pos + 1);
        if (pos2 == std::string_view::npos) {
            query_ = uri.substr(pos + 1);
        } else {
            query_ = uri.substr(pos + 1, pos2 - pos - 1);
            fragment_ = uri.substr(pos2 + 1);
        }
    }
}

void HttpRequest::setHeader(const std::string& key,
                                   const std::string& val) {
    headers_[key] = val;
}

void HttpRequest::setParam (const std::string& key,
                                   const std::string& val) {
    params_[key] = val;
}

void HttpRequest::setCookie(const std::string& key,
                                   const std::string& val) {
    cookies_[key] = val;
}

void HttpRequest::delHeader(const std::string& key) {
    headers_.erase(key);
}

void HttpRequest::delParam (const std::string& key) {
    params_.erase(key);
}

void HttpRequest::delCookie(const std::string& key) {
    cookies_.erase(key);
}

bool HttpRequest::hasHeader(const std::string& key, std::string* val) {
    auto it = headers_.find(key);
    if (it == headers_.end()) {
        return false;
    }
    if (val) {
        *val = it->second;
    }
    return true;
}

bool HttpRequest::hasParam(const std::string& key, std::string* val) {
    auto it = params_.find(key);
    if (it == params_.end()) {
        return false;
    }
    if (val) {
        *val = it->second;
    }
    return true;
}

bool HttpRequest::hasCookie(const std::string& key, std::string* val) {
    auto it = cookies_.find(key);
    if (it == cookies_.end()) {
        return false;
    }
    if (val) {
        *val = it->second;
    }
    return true;
}

std::ostream& HttpRequest::dump(std::ostream& os) const {
    os << HttpMethodToString(method_) << " " << path_ 
       << (query_.empty() ? "" : "?") << query_
       << (fragment_.empty() ? "" : "#") << fragment_
       << " HTTP/" << (uint32_t)(version_ >> 4) << "."
       << (uint32_t)(version_ & 0x0f) << "\r\n";

    if (close_) {
        headers_["connection"] = "close";
    }

    for (auto& [x, y] : headers_) {
        os << x << ": " << y << "\r\n";
    }

    if (!body_.empty()) {
        os << "Content-length " << body_.size() << "\r\n\r\n" << body_;
    } else {
        os << "\r\n";
    }

    return os;
}

void HttpRequest::init() {
    std::string conn = getHeader("connection");
    if (!conn.empty()) {
        if (strncasecmp(conn.c_str(), "close", 5) == 0) {
            close_ = true;
        } else {
            close_ = false;
        }
    }
}

HttpResponse::HttpResponse(uint8_t version, bool close)
    : status_(HttpStatus::OK), version_(version), close_(close) {}

std::string HttpResponse::getHeader(const std::string &key,
                                    const std::string &def) const {
    auto it = headers_.find(key);
    return it == headers_.end() ? def : it->second;
}

void HttpResponse::setHeader(const std::string &key,
                             const std::string &val) {
    headers_[key] = val;
}

void HttpResponse::delHeader(const std::string &key) {
    headers_.erase(key);
}

std::ostream& HttpResponse::dump(std::ostream &os) const {
    os << "HTTP/" << (uint32_t)(version_ >> 4) << "."
       << (uint32_t)(version_ & 0x0f) << " "
       << (int)status_ << " "
       << (reason_.empty() ? HttpStatusToString(status_) : reason_)
       << "\r\n";

    if (close_) {
        headers_["connection"] = "close";
    }

    for (auto& [x, y] : headers_) {
        os << x << ": " << y << "\r\n";
    }

    if (!body_.empty()) {
        os << "content-length: " << body_.size() << "\r\n\r\n"
           << body_;
    } else {
        os << "\r\n";
    }
    return os;
}

} // namespace flexy http