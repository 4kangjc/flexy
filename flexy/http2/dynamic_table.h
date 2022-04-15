#pragma once

#include <string_view>
#include <string>
#include <vector>

namespace flexy::http2 {

class DynamicTable {
public:
    DynamicTable();
    int32_t updata(std::string_view name, std::string_view value);
    int32_t findIndex(std::string_view name) const;
    std::pair<int32_t, bool> findPair(std::string_view name, std::string_view value);
    std::pair<std::string_view, std::string_view> getPair(uint32_t idx) const;
    std::string_view getName(uint32_t idx) const;
    std::string toString() const;

    void sexMaxDataSize(int32_t v) { maxDataSize_ = v; }
public:
    static std::pair<std::string_view, std::string_view> GetStaticHeaders(uint32_t idx);
    static int32_t GetStaticHeadersIndex(std::string_view name);
    static std::pair<int32_t, bool> GetStaticHeadersPair(std::string_view name, std::string_view val);
private:
    int32_t maxDataSize_;
    int32_t dataSize_;
    std::vector<std::pair<std::string, std::string>> datas_;    
};

}
