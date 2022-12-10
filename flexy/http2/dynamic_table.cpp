#include "dynamic_table.h"

#include <array>

namespace flexy::http2 {

#define STATIC_HEADERS(XX)                 \
    XX("", "")                             \
    XX(":authority", "")                   \
    XX(":method", "GET")                   \
    XX(":method", "POST")                  \
    XX(":path", "/")                       \
    XX(":path", "/index.html")             \
    XX(":scheme", "http")                  \
    XX(":scheme", "https")                 \
    XX(":status", "200")                   \
    XX(":status", "204")                   \
    XX(":status", "206")                   \
    XX(":status", "304")                   \
    XX(":status", "400")                   \
    XX(":status", "404")                   \
    XX(":status", "500")                   \
    XX("accept-charset", "")               \
    XX("accept-encoding", "gzip, deflate") \
    XX("accept-language", "")              \
    XX("accept-ranges", "")                \
    XX("accept", "")                       \
    XX("access-control-allow-origin", "")  \
    XX("age", "")                          \
    XX("allow", "")                        \
    XX("authorization", "")                \
    XX("cache-control", "")                \
    XX("content-disposition", "")          \
    XX("content-encoding", "")             \
    XX("content-language", "")             \
    XX("content-length", "")               \
    XX("content-location", "")             \
    XX("content-range", "")                \
    XX("content-type", "")                 \
    XX("cookie", "")                       \
    XX("date", "")                         \
    XX("etag", "")                         \
    XX("expect", "")                       \
    XX("expires", "")                      \
    XX("from", "")                         \
    XX("host", "")                         \
    XX("if-match", "")                     \
    XX("if-modified-since", "")            \
    XX("if-none-match", "")                \
    XX("if-range", "")                     \
    XX("if-unmodified-since", "")          \
    XX("last-modified", "")                \
    XX("link", "")                         \
    XX("location", "")                     \
    XX("max-forwards", "")                 \
    XX("proxy-authenticate", "")           \
    XX("proxy-authorization", "")          \
    XX("range", "")                        \
    XX("referer", "")                      \
    XX("refresh", "")                      \
    XX("retry-after", "")                  \
    XX("server", "")                       \
    XX("set-cookie", "")                   \
    XX("strict-transport-security", "")    \
    XX("transfer-encoding", "")            \
    XX("user-agent", "")                   \
    XX("vary", "")                         \
    XX("via", "")                          \
    XX("www-authenticate", "")

static constexpr std::array<std::pair<std::string_view, std::string_view>, 62>
    s_static_headers{{
#define XX(k, v) {k, v},
        STATIC_HEADERS(XX)
#undef XX
    }};

std::pair<std::string_view, std::string_view> DynamicTable::GetStaticHeaders(
    uint32_t idx) {
    return s_static_headers[idx];
}

int32_t DynamicTable::GetStaticHeadersIndex(std::string_view name) {
    for (size_t i = 1; i < s_static_headers.size(); ++i) {
        if (s_static_headers[i].first == name) {
            return i;
        }
    }
    return -1;
}

std::pair<int32_t, bool> DynamicTable::GetStaticHeadersPair(
    std::string_view name, std::string_view val) {
    std::pair<int32_t, bool> rt = std::make_pair(-1, false);
    for (size_t i = 1; i < s_static_headers.size(); ++i) {
        auto [s_name, s_val] = s_static_headers[i];
        if (s_name == name) {
            if (rt.first == -1) {
                rt.first = i;
            }
        } else {
            continue;
        }
        if (s_val == val) {
            rt.first = i;
            rt.second = true;
        }
    }
    return rt;
}

DynamicTable::DynamicTable() : maxDataSize_(4096), dataSize_(0) {}

int32_t DynamicTable::update(std::string_view name, std::string_view val) {
    int len = name.length() + val.length() + 32;  // why add 32 ?
    int idx = 0;
    while (dataSize_ + len > maxDataSize_ && !datas_.empty()) {
        auto& [d_name, d_val] = datas_[0];
        dataSize_ -= d_name.length() + d_val.length() + 32;
        ++idx;
    }
    datas_.erase(datas_.begin(), datas_.begin() + idx);
    dataSize_ += len;
    datas_.emplace_back(name, val);
    return 0;
}

int32_t DynamicTable::findIndex(std::string_view name) const {
    int32_t idx = GetStaticHeadersIndex(name);
    if (idx == -1) {
        size_t len = datas_.size();
        for (size_t i = 0; i < len; ++i) {
            if (datas_[len - i - 1].first == name) {
                idx = i + 62;
                break;
            }
        }
    }
    return idx;
}

std::pair<int32_t, bool> DynamicTable::findPair(std::string_view name,
                                                std::string_view value) {
    auto rt = GetStaticHeadersPair(name, value);
    if (!rt.second) {
        size_t len = datas_.size();
        for (size_t i = 0; i < datas_.size(); ++i) {
            if (datas_[len - i - 1].first == name) {
                if (rt.first == -1) {
                    rt.first = i + 62;
                }
            } else {
                continue;
            }
            if (datas_[len - i - 1].second == value) {
                rt.first = i + 62;
                rt.second = true;
                break;
            }
        }
    }
    return rt;
}

std::pair<std::string_view, std::string_view> DynamicTable::getPair(
    uint32_t idx) const {
    if (idx < 62) {
        return GetStaticHeaders(idx);
    }
    idx -= 62;
    if (idx < datas_.size()) {
        return datas_[datas_.size() - idx - 1];
    }
    return {"", ""};
}

std::string_view DynamicTable::getName(uint32_t idx) const {
    return getPair(idx).first;
}

}  // namespace flexy::http2