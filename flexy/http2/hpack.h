#pragma once

#include "dynamic_table.h"
#include "flexy/net/bytearray.h"

namespace flexy::http2 {

// https://github.com/halfrost/Halfrost-Field/blob/master/contents/Protocol/HTTP:2_Header-Compression.md#2-string-literal-representation

// 1. 索引 header 字段表示
// 索引 header 字段以 1 位模式 “1” 开头
// 2. 字面 header 字段标识
// (1). 带增量索引的字面 header 字段 以 “01” 2 位模式开头
// (2). 不带索引的字面 header 字段  以 “0000” 4 位模式开头
// (3). 从不索引的字面 header 字段 以 “0001” 4 位模式开

enum class IndexType {
    INDEXED                         = 0,            // Name 和 Value 都在索引表(包括静态表和动态表)
    WITH_INDEXING_INDEXED_NAME      = 1,            // Name 在索引表(包括静态表和动态表)中，Value 需要编码传递，并同时新增到动态表中
    WITH_INDEXING_NEW_NAME          = 2,            // Name 和 Value 都需要编码传递，并同时新增到动态表中
    WITHOUT_INDEXING_INDEXED_NAME   = 3,            // Name 在索引表(包括静态表和动态表)中，Value 需要编码传递，并不新增到动态表中
    WITHOUT_INDEXING_NEW_NAME       = 4,            // Name 和 Value 需要编码传递，并不新增到动态表中
    NEVER_INDEXED_INDEXED_NAME      = 5,            // Name 在索引表(包括静态表和动态表)中，Value 需要编码传递，并永远不新增到动态表中
    NEVER_INDEXED_NEW_NAME          = 6,            // Name 和 Value 需要编码传递，并永远不新增到动态表中
    ERROR                           = 7
};

std::string_view IndexTypeToString(IndexType type);

struct StringHeader {
    union {
        struct {
            uint8_t len : 7;
            uint8_t h   : 1;
        };
        uint8_t h_len;
    };
};

struct FieldHeader {
    union {
        struct {
            uint8_t index : 7;
            uint8_t code  : 1;
        } indexed;
        struct {
            uint8_t index : 6;
            uint8_t code  : 2;
        } with_indexing;
        struct {
            uint8_t index : 4;
            uint8_t code  : 4;
        } other;
        uint8_t type = 0;
    };
};

struct HeaderField {
    IndexType type = IndexType::ERROR;
    bool h_name = false;
    bool h_value = false;
    uint32_t index = 0;
    std::string name;
    std::string value;

    std::string toString() const;
};

class HPack {
public:
    using ptr = std::shared_ptr<HPack>;
    HPack(DynamicTable& table);

    int parse(const ByteArray::ptr& ba, int length);
    int parse(std::string& data);
    int pack(HeaderField* header, const ByteArray::ptr& ba);
    int pack(const std::vector<std::pair<std::string, std::string>>& headers, const ByteArray::ptr& ba);
    int pack(const std::vector<std::pair<std::string, std::string>>& headers, std::string& out);

    std::vector<HeaderField>& getHeaders() { return headers_; }
    static int Pack(HeaderField* header, const ByteArray::ptr& ba);

    std::string toString() const;
public:
    // flag: 将第prefix位填充为1
    template <int32_t preifx, bool flag = false>
    static int WriteVarInt(const ByteArray::ptr& ba, uint64_t value);

    template <int32_t prefix>
    static uint64_t ReadVarInt(const ByteArray::ptr& ba);

    template <int32_t prefix>
    static uint64_t ReadVarInt(const ByteArray::ptr& ba, uint8_t b);

    static std::string ReadString(const ByteArray::ptr& ba);
    static int WriteString(const ByteArray::ptr& ba, const std::string& str, bool h);
private:
    std::vector<HeaderField> headers_;
    DynamicTable& table_;
};

template <int32_t prefix, bool flag>
int HPack::WriteVarInt(const ByteArray::ptr &ba, uint64_t value) {
    static_assert(prefix < 8);
    size_t pos = ba->getPosition();
    uint8_t flags = 0;
    if constexpr (flag) {
        flags = (1 << prefix);
    }
    constexpr uint64_t v = (1 << prefix) - 1;
    if (value < v) {
        ba->writeFuint8(value | flags);
        return 1;
    }
    ba->writeFuint8(v | flags);
    value -= v;
    ba->writeUint64(value);
    return ba->getPosition() - pos;
}

template <int32_t prefix>
uint64_t HPack::ReadVarInt(const ByteArray::ptr &ba) {
    static_assert(prefix < 8);
    uint8_t b = ba->readFuint8();
    return HPack::ReadVarInt<prefix>(ba, b);
}

template <int32_t prefix>
uint64_t HPack::ReadVarInt(const ByteArray::ptr &ba, uint8_t b) {
    static_assert(prefix < 8);
    constexpr uint64_t v = (1 << prefix) - 1;
    b &= v;     // 去除填充位
    if (b < v) {
        return b;
    }
    return ba->readInt64() + b;
}

} // namespace flexy http2