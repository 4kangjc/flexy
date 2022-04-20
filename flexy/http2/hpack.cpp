#include "hpack.h"
#include "flexy/util/log.h"
#include "huffman.h"

namespace flexy::http2 {

static auto g_logger = FLEXY_LOG_NAME("system");

static constexpr std::array<std::string_view, 8> s_index_type_strings = {
        "INDEXED",
        "WITH_INDEXING_INDEXED_NAME",
        "WITH_INDEXING_NEW_NAME",
        "WITHOUT_INDEXING_INDEXED_NAME",
        "WITHOUT_INDEXING_NEW_NAME",
        "NERVER_INDEXED_INDEXED_NAME",
        "NERVER_INDEXED_NEW_NAME",
        "ERROR"
};

std::string_view IndexTypeToSring(IndexType type) {
    auto v = static_cast<uint8_t>(type);
    if (v < s_index_type_strings.size()) {
        return s_index_type_strings[v];
    }
    return fmt::format("UNKNOWN({})", v);
}

std::string HeaderField::toString() const {
    return fmt::format("[header type = {} h_name = {} h_value = {} index = {} name = {} value = {}]",
                       IndexTypeToSring(type), h_name, h_value, index, name, value);
}

HPack::HPack(DynamicTable &table)
    : table_(table)
{}

std::string HPack::ReadString(const ByteArray::ptr &ba) {
    uint8_t type = ba->readFuint8();
    int len = ReadVarInt<7>(ba, type);
    std::string data;
    if (len) {
        data.resize(len);
        ba->read(data.data(), len);
        if (type & 0x80) {              // 标志位 'H' 指示字符串的八位字节是否经过霍夫曼编码
            std::string out;
            huffman::DecodeString(data, out);
            return out;
        }
    }
    return data;
}

int HPack::WriteString(const ByteArray::ptr &ba, const std::string &str, bool h) {
    int pos = ba->getPosition();
    if (h) {
        std::string new_str;
        huffman::EncodeString(str, new_str, 0);
        WriteVarInt<7, true>(ba, new_str.length());
        ba->write(new_str.data(), new_str.size());
    } else {
        WriteVarInt<7>(ba, str.size());
        ba->write(str.data(), str.length());
    }
    return ba->getPosition() - pos;
}

int HPack::parse(std::string &data) {
    auto ba = std::make_shared<ByteArray>(data.data(), data.size(), false);
    return parse(ba, data.size());
}

int HPack::parse(const ByteArray::ptr &ba, int length) {
    int parsed = 0;
    int pos = ba->getPosition();
    while (parsed < length) {
        HeaderField header;
        uint8_t type = ba->readFuint8();
        if (type & 0x80) {                  // 索引 header 字段以 1 位模式 “1” 开头
            uint32_t idx = ReadVarInt<7>(ba, type);
            header.type = IndexType::INDEXED;
            header.index = idx;
        } else {
            if (type & 0x40) {           // 带增量索引的字面 header 字段 以 “01” 2 位模式开头
                uint32_t idx = ReadVarInt<6>(ba, type);
                header.type = idx > 0 ? IndexType::WITH_INDEXING_INDEXED_NAME : IndexType::WITH_INDEXING_NEW_NAME;
                header.index = idx;
            } else if ((type & 0xf0) == 0) {    // 不带索引的字面 header 字段  以 “0000” 4 位模式开头
                uint32_t idx = ReadVarInt<4>(ba, type);
                header.type = idx > 0 ? IndexType::WITHOUT_INDEXING_INDEXED_NAME : IndexType::WITHOUT_INDEXING_NEW_NAME;
                header.index = idx;
            } else if (type & 0x10) {           // 从不索引的字面 header 字段 以 “0001” 4 位模式开
                uint32_t idx = ReadVarInt<4>(ba, type);
                header.type = idx > 0 ? IndexType::NEVER_INDEXED_INDEXED_NAME : IndexType::NEVER_INDEXED_NEW_NAME;
                header.index = idx;
            } else {
                return -1;
            }

            if (header.index > 0) {
                header.value = ReadString(ba);
            } else {
                header.name = ReadString(ba);
                header.value = ReadString(ba);
            }
        }

        if (header.type == IndexType::INDEXED) {
            auto [x, y] = table_.getPair(header.index);
            header.name = x;
            header.value = y;
        } else if (header.index > 0) {
            auto [x, _] = table_.getPair(header.index);
            header.name = x;
        }

        if (header.type == IndexType::WITH_INDEXING_INDEXED_NAME) {
            table_.update(header.name, header.value);
        } else if (header.type == IndexType::WITH_INDEXING_NEW_NAME) {
            table_.update(header.name, header.value);
        }
        headers_.push_back(std::move(header));
        parsed = ba->getPosition() - pos;
    }

    return parsed;
}

int HPack::Pack(HeaderField *header, const ByteArray::ptr &ba) {
    int pos = ba->getPosition();

    switch (header->type) {
        case IndexType::INDEXED: {
            WriteVarInt<7, true>(ba, header->index);
            break;
        }
        case IndexType::WITH_INDEXING_INDEXED_NAME: {
            WriteVarInt<6, true>(ba, header->index);
            WriteString(ba, header->value, header->h_value);
            break;
        }
        case IndexType::WITH_INDEXING_NEW_NAME: {
            WriteVarInt<6, true>(ba, header->index);        // header->index == 0
            WriteString(ba, header->name, header->h_name);
            WriteString(ba, header->value, header->h_value);
            break;
        }
        case IndexType::WITHOUT_INDEXING_NEW_NAME: {
            WriteVarInt<4>(ba, header->index);
            WriteString(ba, header->value, header->h_value);
            break;
        }
        case IndexType::WITHOUT_INDEXING_INDEXED_NAME: {
            WriteVarInt<4>(ba, header->index);
            WriteString(ba, header->name, header->h_value);
            WriteString(ba, header->value, header->h_value);
            break;
        }
        case IndexType::NEVER_INDEXED_INDEXED_NAME: {
            WriteVarInt<4, true>(ba, header->index);
            WriteString(ba, header->value, header->h_value);
            break;
        }
        case IndexType::NEVER_INDEXED_NEW_NAME: {
            WriteVarInt<4, true>(ba, header->index);
            WriteString(ba, header->name, header->h_name);
            WriteString(ba, header->value, header->h_value);
            break;
        }
        case IndexType::ERROR:
            FLEXY_LOG_INFO(g_logger) << "HPack::pack ERROR";
            break;
    }
    return ba->getPosition() - pos;
}

int HPack::pack(HeaderField *header, const ByteArray::ptr &ba) {
    auto ret = HPack::Pack(header, ba);
    headers_.push_back(std::move(*header));
    return ret;
}

int HPack::pack(const std::vector<std::pair<std::string, std::string>> &headers, std::string &out) {
    auto ba = std::make_shared<ByteArray>();
    int rt = pack(headers, ba);
    ba->setPosition(0);
    ba->toString().swap(out);
    return rt;
}

int HPack::pack(const std::vector<std::pair<std::string, std::string>> &headers, const ByteArray::ptr &ba) {
    int rt = 0;
    for (auto& [x, y] : headers) {
        HeaderField h;
        auto [idx, ok] = table_.findPair(x, y);
        if (ok) {
            h.type = IndexType::INDEXED;
            h.index = idx;
        } else if (idx > 0) {
            h.type = IndexType::WITH_INDEXING_INDEXED_NAME;
            h.index = idx;
            h.h_value = huffman::ShouldEncode(y);
            h.name = x;     // headers.push_back 需要
            h.value = y;
            table_.update(x, y);
        } else {
            h.type = IndexType::WITH_INDEXING_NEW_NAME;
            h.index = 0;
            h.h_name = huffman::ShouldEncode(x);
            h.name = x;
            h.h_value = huffman::ShouldEncode(y);
            h.value = y;
            table_.update(x, y);
        }

        rt += pack(&h, ba);
    }
    return rt;
}

std::string HPack::toString() const {
    auto ret = fmt::format("[HPack size = {}]", headers_.size());
    int i = 0;
    for (auto& header : headers_) {
        ret += fmt::format("\t{}\t:\t{}", i++, header.toString());
    }
    return ret;
}

} // namespace flexy http2