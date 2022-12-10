#include "frame.h"
#include "flexy/util/log.h"

namespace flexy::http2 {

static auto g_logger = FLEXY_LOG_NAME("system");

std::string FrameHeader::toString() const {
    return fmt::format(
        "[FrameHeader length = {} type = {} flags = {} r = {} identifier = {}]",
        +length, FrameTypeToString((FrameType)type),
        FrameFlagToString(type, flags), (int)r, identifier);
}

bool FrameHeader::writeTo(const ByteArray::ptr &ba) const {
    try {
        ba->writeFuint32(len_type);
        ba->writeFuint8(flags);
        ba->writeFuint32(r_id);
        return true;
    } catch (...) {
        FLEXY_LOG_WARN(g_logger) << "write FrameHeader fail, " << toString();
    }
    return false;
}

bool FrameHeader::readFrom(const ByteArray::ptr &ba) {
    try {
        len_type = ba->readFuint32();
        flags = ba->readFuint8();
        r_id = ba->readFuint32();
        return true;
    } catch (...) {
        FLEXY_LOG_WARN(g_logger) << "read FrameHeader fail" << toString();
    }
    return false;
}

std::string Frame::toString() const {
    return header.toString() + (data ? data->toString() : "");
}

std::string DataFrame::toString() const {
    if (pad) {
        return fmt::format("[DataFrame pad = {} data.size = {}]", pad,
                           data.size());
    } else {
        return fmt::format("[DataFrame data.size = {}]", data.size());
    }
}

bool DataFrame::writeTo(const ByteArray::ptr &ba,
                        const FrameHeader &header) const {
    try {
        if (header.flags & static_cast<uint8_t>(FrameFlagData::PADDED)) {
            ba->writeFuint8(pad);
            ba->write(data.data(), data.size());
            ba->write(padding.data(), padding.size());
        } else {
            ba->write(data.data(), data.size());
        }
        return true;
    } catch (...) {
        FLEXY_LOG_WARN(g_logger) << "write DataFrame fail, " << toString();
    }
    return false;
}

bool DataFrame::readFrom(const ByteArray::ptr &ba, const FrameHeader &header) {
    try {
        if (header.flags & static_cast<uint8_t>(FrameFlagData::PADDED)) {
            pad = ba->readFuint8();
            data.resize(header.length - pad -
                        1);  // 减掉padding的长度和pad的长度
            ba->read(data.data(), data.size());
            padding.resize(pad);
            ba->read(padding.data(), pad);
        } else {
            data.resize(header.length);
            ba->read(data.data(), data.size());
        }
        return true;
    } catch (...) {
        FLEXY_LOG_WARN(g_logger) << "read DataFrame fail, " << toString();
    }
    return false;
}

std::string PriorityFrame::toString() const {
    return fmt::format(
        "[PriorityFrame exclusive = {} stream_dep = {} wight = {}]", exclusive,
        stream_dep, weight);
}

bool PriorityFrame::writeTo(const ByteArray::ptr &ba,
                            const FrameHeader &header) const {
    try {
        ba->writeFuint32(e_stream_dep);
        ba->writeFuint8(weight);
        return true;
    } catch (...) {
        FLEXY_LOG_WARN(g_logger) << "write PriorityFrame fail, " << toString();
    }
    return false;
}

bool PriorityFrame::readFrom(const ByteArray::ptr &ba,
                             const FrameHeader &header) {
    try {
        e_stream_dep = ba->readFuint32();
        weight = ba->readFuint8();
        return true;
    } catch (...) {
        FLEXY_LOG_WARN(g_logger) << "read PriorityFrame fail, " << toString();
    }
    return false;
}

std::string HeadersFrame::toString() const {
    return fmt::format("[HeadersFrame pad = {} data.size = {}]", pad,
                       data.size());
}

bool HeadersFrame::writeTo(const ByteArray::ptr &ba,
                           const FrameHeader &header) const {
    try {
        if (header.flags & static_cast<uint8_t>(FrameFlagHeaders::PADDED)) {
            ba->writeFuint8(pad);
        }
        if (header.flags & static_cast<uint8_t>(FrameFlagHeaders::PRIORITY)) {
            priority.writeTo(ba, header);
        }
        if (hpack && !kvs.empty()) {
            hpack->pack(kvs, data);
        }
        ba->write(data.data(), data.size());
        if (header.flags & static_cast<uint8_t>(FrameFlagHeaders::PADDED)) {
            ba->write(padding.data(), padding.size());
        }
        return true;
    } catch (...) {
        FLEXY_LOG_WARN(g_logger) << "write HeadersFrame fail, " << toString();
    }
    return false;
}

bool HeadersFrame::readFrom(const ByteArray::ptr &ba,
                            const FrameHeader &header) {
    try {
        int len = header.length;
        if (header.flags & static_cast<uint8_t>(FrameFlagHeaders::PADDED)) {
            pad = ba->readFuint8();
            len -= 1 + pad;
        }
        if (header.flags & static_cast<uint8_t>(FrameFlagHeaders::PRIORITY)) {
            priority.readFrom(ba, header);
            len -= PriorityFrame::SIZE;
        }
        //
        data.resize(len);
        ba->read(data.data(), data.size());
        if (header.flags & static_cast<uint8_t>(FrameFlagHeaders::PADDED)) {
            padding.resize(pad);
            ba->read(padding.data(), padding.size());
        }

        return true;
    } catch (...) {
        FLEXY_LOG_WARN(g_logger) << "read HeadersFrame fail, " << toString();
    }
    return false;
}

std::string RstStreamFrame::toString() const {
    return fmt::format("[RstStream error_code = {}]", error_code);
}

bool RstStreamFrame::writeTo(const ByteArray::ptr &ba,
                             const FrameHeader &header) const {
    try {
        ba->writeFuint32(error_code);
        return true;
    } catch (...) {
        FLEXY_LOG_WARN(g_logger) << "read RstStreamFrame fail, " << toString();
    }
    return false;
}

bool RstStreamFrame::readFrom(const ByteArray::ptr &ba,
                              const FrameHeader &header) {
    try {
        error_code = ba->readFuint32();
        return true;
    } catch (...) {
        FLEXY_LOG_WARN(g_logger) << "read RstStreamFrame fail, " << toString();
    }
    return false;
}

static constexpr std::array<std::string_view, 6> s_settings_string = {
    "",
    "HEADER_TABLE_SIZE",
    "MAX_CONCURRENT_STREAMS",
    "INITIAL_WINDOW_SIZE",
    "MAX_FRAME_SIZE",
    "MAX_HEADER_LIST_SIZE"};

std::string_view SettingsFrame::SettingsToString(Settings s) {
    auto idx = static_cast<uint32_t>(s);
    if (idx > s_settings_string.size()) {
        // TODO log
        return "UNKNOWN";
    }
    return s_settings_string[idx];
}

std::string SettingsItem::toString() const {
    return fmt::format(
        "[SettingsFrame identifier = {} value = {}]",
        SettingsFrame::SettingsToString((SettingsFrame::Settings)identifier),
        value);
}

bool SettingsItem::writeTo(const ByteArray::ptr &ba) const {
    ba->writeFuint16(identifier);
    ba->writeFuint32(value);
    return true;
}

bool SettingsItem::readFrom(const ByteArray::ptr &ba) {
    identifier = ba->readFuint16();
    value = ba->readFuint32();
    return true;
}

bool SettingsFrame::writeTo(const ByteArray::ptr &ba,
                            const FrameHeader &header) const {
    try {
        for (const auto &i : items) {
            i.writeTo(ba);
        }
        return true;
    } catch (...) {
        FLEXY_LOG_WARN(g_logger) << "write SettingsFrame fail, " << toString();
    }
    return false;
}

bool SettingsFrame::readFrom(const ByteArray::ptr &ba,
                             const FrameHeader &header) {
    try {
        uint32_t size = header.length / sizeof(SettingsItem);
        items.resize(size);
        for (uint32_t i = 0; i < size; ++i) {
            items[i].readFrom(ba);
        }
        return true;
    } catch (...) {
        FLEXY_LOG_WARN(g_logger) << "read SettingsFrame fail, " << toString();
    }
    return false;
}

std::string SettingsFrame::toString() const {
    return fmt::format("[SettingsFrame size = {} items = [{}]]", items.size(),
                       [this]() {
                           std::string ret;
                           ret.reserve(50 * items.size());  // 粗略估计大小
                           for (const auto &i : items) {
                               ret += i.toString();
                           }
                           return ret;
                       }());
}

std::string PushPromisedFrame::toString() const {
    if (pad) {
        return fmt::format("[PushPromisedFrame pad = {} data.size = {}]", pad,
                           data.size());
    } else {
        return fmt::format("[PushPromisedFrame data.size = {}]", data.size());
    }
}

bool PushPromisedFrame::writeTo(const ByteArray::ptr &ba,
                                const FrameHeader &header) const {
    try {
        if (header.flags & static_cast<uint8_t>(FrameFlagPromise::PADDED)) {
            ba->writeFuint8(pad);
            ba->writeFuint32(r_stream_id);
            ba->write(data.data(), data.size());
            ba->write(padding.data(), padding.size());
        } else {
            ba->writeFuint32(r_stream_id);
            ba->write(data.data(), data.size());
        }
        return true;
    } catch (...) {
        FLEXY_LOG_WARN(g_logger)
            << "write PushPromiseFrame fail, " << toString();
    }
    return false;
}

bool PushPromisedFrame::readFrom(const ByteArray::ptr &ba,
                                 const FrameHeader &header) {
    try {
        if (header.flags & static_cast<uint8_t>(FrameFlagPromise::PADDED)) {
            pad = ba->readFuint8();
            r_stream_id = ba->readFuint32();
            data.resize(header.length - pad - 5);
            ba->read(data.data(), data.size());
            padding.resize(pad);
            ba->read(padding.data(), padding.size());
        } else {
            r_stream_id = ba->readFuint32();
            data.resize(header.length - 4);
            ba->read(data.data(), data.size());
        }
        return true;
    } catch (...) {
        FLEXY_LOG_WARN(g_logger)
            << "read PushPromiseFrame fail, " << toString();
    }
    return false;
}

std::string PingFrame::toString() const {
    return fmt::format("[PingFrame uint64 = {}]", uint64);
}

bool PingFrame::writeTo(const ByteArray::ptr &ba,
                        const FrameHeader &header) const {
    try {
        ba->write(data, 8);
        return true;
    } catch (...) {
        FLEXY_LOG_WARN(g_logger) << "write PingFrame fail, " << toString();
    }
    return false;
}

bool PingFrame::readFrom(const ByteArray::ptr &ba, const FrameHeader &header) {
    try {
        ba->read(data, 8);
        return true;
    } catch (...) {
        FLEXY_LOG_WARN(g_logger) << "read PingFrame fail, " << toString();
    }
    return false;
}

std::string GoAwayFrame::toString() const {
    return fmt::format(
        "[GoAwayFrame r = {} last_stream_id = {} error_code = {} debug.size = "
        "{}",
        r, last_stream_id, error_code, data.size());
}

bool GoAwayFrame::readFrom(const ByteArray::ptr &ba,
                           const FrameHeader &header) {
    try {
        r_last_stream_id = ba->readFint32();
        error_code = ba->readFuint32();
        if (header.length > 8) {
            data.resize(header.length - 8);
            ba->read(data.data(), data.size());
        }
        return true;
    } catch (...) {
        FLEXY_LOG_WARN(g_logger) << "read GoAwayFrame fail, " << toString();
    }
    return false;
}

bool GoAwayFrame::writeTo(const ByteArray::ptr &ba,
                          const FrameHeader &header) const {
    try {
        ba->writeFuint32(r_last_stream_id);
        ba->writeFuint32(error_code);
        if (!data.empty()) {
            ba->write(data.data(), data.size());
        }
        return true;
    } catch (...) {
        FLEXY_LOG_WARN(g_logger) << "write GoAway fail, " << toString();
    }
    return false;
}

std::string WindowUpdateFrame::toString() const {
    return fmt::format("[WindowUpdateFrame r = {} increment = {}]", r,
                       increment);
}

bool WindowUpdateFrame::writeTo(const ByteArray::ptr &ba,
                                const FrameHeader &header) const {
    try {
        ba->writeFuint32(r_increment);
        return true;
    } catch (...) {
        FLEXY_LOG_WARN(g_logger)
            << "write WindowUpdateFrame fail, " << toString();
    }
    return false;
}

bool WindowUpdateFrame::readFrom(const ByteArray::ptr &ba,
                                 const FrameHeader &header) {
    try {
        r_increment = ba->readFuint32();
        return true;
    } catch (...) {
        FLEXY_LOG_WARN(g_logger)
            << "read WindowUpdateFrame fail, " << toString();
    }
    return false;
}

static constexpr std::array<std::string_view, 8> s_frame_types = {
    "DATA",         "HEADERS", "RST_STREAM",    "SETTINGS",
    "PUSH_PROMISE", "GOAWAY",  "WINDOW_UPDATE", "CONTINUATION"};

std::string_view FrameTypeToString(FrameType type) {
    auto v = static_cast<uint8_t>(type);
    if (v > 9) {
        // TODO log v
        return "UNKNOWN";
    }
    return s_frame_types[v];
}

std::string FrameFlagDataToString(FrameFlagData flag) {
    auto x = static_cast<uint8_t>(flag);
    if (x > 9) {
        return fmt::format("UNKNOWN({})", x);
    }
    std::string ret;

    while (x != 0) {
        switch (x & (-x)) {  // lowbit 获取二进制最右边的1
            case 0x1:
                ret += "END_STREAM|";
                break;
            case 0x8:
                ret += "PADDED|";
                break;
            default:
                return fmt::format("UNKOWN({})", x);
        }
        x &= (x - 1);  // 去除二进制最右边的1
    }

    if (ret.empty()) {
        return "0";
    }
    ret.pop_back();  // pop '|'
    return ret;
}

std::string FrameFlagHeadersToString(FrameFlagHeaders flag) {
    auto x = static_cast<uint8_t>(flag);
    if (x > 45) {
        return fmt::format("UNKNOWN({})", x);
    }
    std::string ret;

    while (x != 0) {
        switch (x & (-x)) {  // lowbit 获取二进制最右边的1
            case 0x1:
                ret += "END_STREAM|";
                break;
            case 0x4:
                ret += "END_HEADERS|";
                break;
            case 0x8:
                ret += "PADDED|";
                break;
            case 0x20:
                ret += "PRIORITY|";
            default:
                return fmt::format("UNKOWN({})", x);
        }
        x &= (x - 1);  // 去除二进制最右边的1
    }

    if (ret.empty()) {
        return "0";
    }
    ret.pop_back();  // pop '|'
    return ret;
}

std::string FrameFlagSettingsToString(FrameFlagSettings flag) {
    auto x = static_cast<uint8_t>(flag);
    if (x > 1) {
        return fmt::format("UNKNOWN({})", x);
    } else if (x == 0) {
        return "0";
    }

    return "ACK";
}

std::string FrameFlagPingToString(FrameFlagPing flag) {
    auto x = static_cast<uint8_t>(flag);
    if (x > 1) {
        return fmt::format("UNKNOWN({})", x);
    } else if (x == 0) {
        return "0";
    }

    return "ACK";
}

std::string FrameFlagContinuationToString(FrameFlagContinuation flag) {
    auto x = static_cast<uint8_t>(flag);
    if (x > 0x4) {
        return fmt::format("UNKNOWN({})", x);
    } else if (x == 0) {
        return "0";
    }

    return "END_HEADERS";
}

std::string FrameFlagPromiseToString(FrameFlagPromise flag) {
    auto x = static_cast<uint8_t>(flag);
    if (x > 9) {
        return fmt::format("UNKNOWN({})", x);
    }
    std::string ret;

    while (x != 0) {
        switch (x & (-x)) {  // lowbit 获取二进制最右边的1
            case 0x1:
                ret += "END_STREAM|";
                break;
            case 0x8:
                ret += "PADDED|";
                break;
            default:
                return fmt::format("UNKOWN({})", x);
        }
        x &= (x - 1);  // 去除二进制最右边的1
    }

    if (ret.empty()) {
        return "0";
    }
    ret.pop_back();  // pop '|'
    return ret;
}

std::string FrameFlagToString(uint8_t type, uint8_t flag) {
    switch ((FrameType)type) {
        case FrameType::DATA:
            return FrameFlagDataToString((FrameFlagData)flag);
        case FrameType::HEADERS:
            return FrameFlagHeadersToString((FrameFlagHeaders)flag);
        case FrameType::SETTINGS:
            return FrameFlagSettingsToString((FrameFlagSettings)flag);
        case FrameType::PING:
            return FrameFlagPingToString((FrameFlagPing)flag);
        case FrameType::CONTINUATION:
            return FrameFlagContinuationToString((FrameFlagContinuation)flag);
        case FrameType::PUSH_PROMISE:
            return FrameFlagPromiseToString((FrameFlagPromise)flag);
        default:
            return flag ? "UNKNOWN(" + std::to_string(flag) + ")" : "0";
    }
}

std::string FrameRToString(FrameR r) {
    return (r == FrameR::SET) ? "SET" : "UNSET";
}

Frame::ptr FrameCodec::parseFrom(Stream *stream) {
    try {
        auto frame = std::make_shared<Frame>();
        auto ba = std::make_shared<ByteArray>();
        auto rt = stream->readFixSize(ba, FrameHeader::SIZE);
        if (rt <= 0) {
            FLEXY_LOG_INFO(g_logger) << "recv frame header fail, rt = " << rt
                                     << " " << strerror(errno);
            return nullptr;
        }
        ba->setPosition(0);
        if (!frame->header.readFrom(ba)) {
            FLEXY_LOG_INFO(g_logger) << "parse frame header fail";
            return nullptr;
        }
        if (frame->header.length > 0) {
            ba->setPosition(0);
            auto r = stream->readFixSize(ba, frame->header.length);
            if (r <= 0) {
                FLEXY_LOG_INFO(g_logger) << "recv frame body fail, rt = " << r
                                         << " " << strerror(errno);
                return nullptr;
            }
            ba->setPosition(0);
        }

        switch ((FrameType)frame->header.type) {
            case FrameType::DATA: {
                frame->data = std::make_shared<DataFrame>();
                if (!frame->data->readFrom(ba, frame->header)) {
                    FLEXY_LOG_INFO(g_logger) << "parse DataFrame fail";
                    return nullptr;
                }
                break;
            }
            case FrameType::HEADERS: {
                frame->data = std::make_shared<HeadersFrame>();
                if (!frame->data->readFrom(ba, frame->header)) {
                    FLEXY_LOG_INFO(g_logger) << "parse HeadersFrame fail";
                    return nullptr;
                }
                break;
            }
            case FrameType::PRIORITY: {
                frame->data = std::make_shared<PriorityFrame>();
                if (!frame->data->readFrom(ba, frame->header)) {
                    FLEXY_LOG_INFO(g_logger) << "parse PriorityFrame fail";
                    return nullptr;
                }
                break;
            }
            case FrameType::RST_STREAM: {
                frame->data = std::make_shared<RstStreamFrame>();
                if (!frame->data->readFrom(ba, frame->header)) {
                    FLEXY_LOG_INFO(g_logger) << "parse RstStreamFrame fail";
                    return nullptr;
                }
                break;
            }
            case FrameType::SETTINGS: {
                frame->data = std::make_shared<SettingsFrame>();
                if (!frame->data->readFrom(ba, frame->header)) {
                    FLEXY_LOG_INFO(g_logger) << "parse SettingsFrame fail";
                    return nullptr;
                }
                break;
            }
            case FrameType::PUSH_PROMISE: {
                frame->data = std::make_shared<PushPromisedFrame>();
                if (!frame->data->readFrom(ba, frame->header)) {
                    FLEXY_LOG_INFO(g_logger) << "parse PushPromisedFrame fail";
                    return nullptr;
                }
                break;
            }
            case FrameType::PING: {
                frame->data = std::make_shared<PingFrame>();
                if (!frame->data->readFrom(ba, frame->header)) {
                    FLEXY_LOG_INFO(g_logger) << "parse PingFrame fail";
                    return nullptr;
                }
                break;
            }
            case FrameType::GOAWAY: {
                frame->data = std::make_shared<GoAwayFrame>();
                if (!frame->data->readFrom(ba, frame->header)) {
                    FLEXY_LOG_INFO(g_logger) << "parse GoAwayFrame fail";
                    return nullptr;
                }
                break;
            }
            case FrameType::WINDOW_UPDATE: {
                frame->data = std::make_shared<WindowUpdateFrame>();
                if (!frame->data->readFrom(ba, frame->header)) {
                    FLEXY_LOG_INFO(g_logger) << "parse WindowUpdateFrame fail";
                    return nullptr;
                }
                break;
            }
            case FrameType::CONTINUATION: {
                //
                break;
            }
            default: {
                FLEXY_LOG_WARN(g_logger)
                    << "invalid FrameType: " << (uint32_t)frame->header.type;
                break;
            }
        }
        return frame;
    } catch (std::exception &e) {
        FLEXY_LOG_ERROR(g_logger)
            << "FrameCodec parseFrom except: " << e.what();
    } catch (...) {
        FLEXY_LOG_ERROR(g_logger) << "FrameCodec parseFrom except";
    }
    return nullptr;
}

size_t FrameCodec::serializeTo(Stream *stream, const Frame::ptr &frame) {
    FLEXY_LOG_DEBUG(g_logger) << "serializeTo " << frame->toString();
    auto ba = std::make_shared<ByteArray>();
    frame->header.writeTo(ba);
    if (frame->data) {
        if (!frame->data->writeTo(ba, frame->header)) {
            FLEXY_LOG_ERROR(g_logger)
                << "FrameCodec serializeTo fail, type = "
                << FrameTypeToString((FrameType)frame->header.type);
            return -1;
        }
        int pos = ba->getPosition();
        ba->setPosition(0);
        frame->header.length = pos - FrameHeader::SIZE;
        frame->header.writeTo(ba);  // 覆盖掉之前写入的`FrameHeader`
    }
    ba->setPosition(0);
    auto rt = stream->writeFixSize(ba, ba->getReadSize());
    if (rt <= 0) {
        FLEXY_LOG_ERROR(g_logger) << "FrameCodec serializeTo fail, rt = " << rt
                                  << " errno = " << errno;
        return -2;
    }
    return ba->getSize();
}

}  // namespace flexy::http2