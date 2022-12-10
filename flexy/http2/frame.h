#pragma once

#include "flexy/stream/stream.h"
#include "hpack.h"

namespace flexy::http2 {

// https://github.com/halfrost/Halfrost-Field/blob/master/contents/Protocol/HTTP:2-HTTP-Frames-Definitions.md#2-defined-settings-parameters

#pragma pack(1)

enum class FrameType {
    DATA = 0x0,
    HEADERS = 0x1,
    PRIORITY = 0x2,
    RST_STREAM = 0x3,
    SETTINGS = 0x4,
    PUSH_PROMISE = 0x5,
    PING = 0x6,
    GOAWAY = 0x7,
    WINDOW_UPDATE = 0x8,
    CONTINUATION = 0x9,
};

enum class FrameFlagData { END_STREAM = 0x1, PADDED = 0x8 };

enum class FrameFlagHeaders {
    END_STREAM = 0x1,
    END_HEADERS = 0x4,
    PADDED = 0x8,
    PRIORITY = 0x20
};

enum class FrameFlagSettings { ACK = 0x1 };

enum class FrameFlagPing { ACK = 0x1 };

enum class FrameFlagContinuation { END_HEADERS = 0x4 };

enum class FrameFlagPromise { END_HEADERS = 0x4, PADDED = 0x8 };

enum class FrameR { UNSET = 0x0, SET = 0x1 };

// https://github.com/halfrost/Halfrost-Field/blob/master/contents/Protocol/HTTP:2-HTTP-Frames.md

/*
HTTP2 frame 格式
+-----------------------------------------------+
|                 Length (24)                   |
+---------------+---------------+---------------+
|   Type (8)    |   Flags (8)   |
+-+-------------+---------------+-------------------------------+
|R|                 Stream Identifier (31)                      |
+=+=============================================================+
|                   Frame Payload (0...)                      ...
+---------------------------------------------------------------+
*/

/*
Length：
帧有效负载的长度表示为无符号的 24 位整数。除非接收方为 SETTINGS_MAX_FRAME_SIZE
设置了较大的值，否则不得发送大于2 ^ 14（16,384）的值。 帧头的 9
个八位字节不包含在此长度值中。

Type：
这 8
位用来表示帧类型的。帧类型确定帧的格式和语义。实现方必须忽略并丢弃任何类型未知的帧。

Flags：
这个字段是为特定于帧类型的布尔标志保留的 8
位字段，为标志分配特定于指示帧类型的语义。没有为特定帧类型定义语义的标志必须被忽略，并且必须在发送时保持未设置
(0x0)

常用的标志位有 END_HEADERS 表示头数据结束，相当于 HTTP/1
里头后的空行（“\r\n”），END_STREAM 表示单方向数据发送结束（即 EOS，End of
Stream）， 相当于 HTTP/1 里 Chunked 分块结束标志（“0\r\n\r\n”）。

R：
保留的 1 位字段。该位的语义未定义，发送时必须保持未设置 (0x0)，接收时必须忽略。

Stream Identifier：
流标识符 (参见 第 5.1.1 节)，表示为无符号 31 位整数。值 0x0
保留用于与整个连接相关联的帧，而不是单个流。
*/

struct FrameHeader {
    static constexpr uint32_t SIZE = 9;
    using ptr = std::shared_ptr<FrameHeader>;
    union {
        struct {
            uint8_t type;
            uint32_t length : 24;
        };
        uint32_t len_type;
    };
    uint8_t flags = 0;
    union {
        struct {
            uint32_t identifier : 31;
            uint32_t r : 1;
        };
        uint32_t r_id;
    };

    [[nodiscard]] std::string toString() const;
    bool writeTo(const ByteArray::ptr& ba) const;
    bool readFrom(const ByteArray::ptr& ba);
};

class IFrame {
public:
    using ptr = std::shared_ptr<IFrame>;

    virtual ~IFrame() = default;
    [[nodiscard]] virtual std::string toString() const = 0;
    virtual bool writeTo(const ByteArray::ptr& ba,
                         const FrameHeader& header) const = 0;
    virtual bool readFrom(const ByteArray::ptr& ba,
                          const FrameHeader& header) = 0;
};

struct Frame {
    using ptr = std::shared_ptr<Frame>;
    FrameHeader header;
    IFrame::ptr data;

    [[nodiscard]] std::string toString() const;
};

/*
 +---------------+
 |Pad Length? (8)|
 +---------------+-----------------------------------------------+
 |                            Data (*)                         ...
 +---------------------------------------------------------------+
 |                           Padding (*)                       ...
 +---------------------------------------------------------------+
*/

struct DataFrame : public IFrame {
    using ptr = std::shared_ptr<DataFrame>;
    uint8_t pad = 0;  // Flag & FrameFlagDate::PADDED
    std::string data;
    std::string padding;

    [[nodiscard]] std::string toString() const override;
    bool writeTo(const ByteArray::ptr& ba,
                 const FrameHeader& header) const override;
    bool readFrom(const ByteArray::ptr& ba, const FrameHeader& header) override;
};

/*
 +-+-------------------------------------------------------------+
 |E|                  Stream Dependency (31)                     |
 +-+-------------+-----------------------------------------------+
 |   Weight (8)  |
 +-+-------------+
*/

struct PriorityFrame : public IFrame {
    using ptr = std::shared_ptr<PriorityFrame>;
    static constexpr uint32_t SIZE = 5;
    union {
        struct {
            uint32_t stream_dep : 31;
            uint32_t exclusive : 1;
        };
        uint32_t e_stream_dep = 0;
    };
    uint8_t weight = 0;

    [[nodiscard]] std::string toString() const override;
    bool writeTo(const ByteArray::ptr& ba,
                 const FrameHeader& header) const override;
    bool readFrom(const ByteArray::ptr& ba, const FrameHeader& header) override;
};

/*
 +---------------+
 |Pad Length? (8)|
 +-+-------------+-----------------------------------------------+
 |E|                 Stream Dependency? (31)                     |
 +-+-------------+-----------------------------------------------+
 |  Weight? (8)  |
 +-+-------------+-----------------------------------------------+
 |                   Header Block Fragment (*)                 ...
 +---------------------------------------------------------------+
 |                           Padding (*)                       ...
 +---------------------------------------------------------------+
 */

struct HeadersFrame : public IFrame {
    using ptr = std::shared_ptr<HeadersFrame>;
    uint8_t pad = 0;           // Flag & FrameFlagHeaders::PADDED
    PriorityFrame priority;    // Flag & FrameFlagHeaders::PRIORITY
    mutable std::string data;  // hpack
    std::string padding;
    HPack::ptr hpack;
    std::vector<std::pair<std::string, std::string>> kvs;

    [[nodiscard]] std::string toString() const override;
    bool writeTo(const ByteArray::ptr& ba,
                 const FrameHeader& header) const override;
    bool readFrom(const ByteArray::ptr& ba, const FrameHeader& header) override;
};

/*
 +---------------------------------------------------------------+
 |                        Error Code (32)                        |
 +---------------------------------------------------------------+
*/

struct RstStreamFrame : public IFrame {
    using ptr = std::shared_ptr<RstStreamFrame>;
    static constexpr uint32_t SIZE = 4;
    uint32_t error_code = 0;

    [[nodiscard]] std::string toString() const override;
    bool writeTo(const ByteArray::ptr& ba,
                 const FrameHeader& header) const override;
    bool readFrom(const ByteArray::ptr& ba, const FrameHeader& header) override;
};

/*
 +-------------------------------+
 |       Identifier (16)         |
 +-------------------------------+-------------------------------+
 |                        Value (32)                             |
 +---------------------------------------------------------------+
*/

struct SettingsItem {
    SettingsItem(uint16_t id = 0, uint32_t v = 0) : identifier(id), value(v) {}

    uint16_t identifier = 0;
    uint32_t value = 0;

    [[nodiscard]] std::string toString() const;
    bool writeTo(const ByteArray::ptr& ba) const;
    bool readFrom(const ByteArray::ptr& ba);
};

struct SettingsFrame : public IFrame {
    using ptr = std::shared_ptr<SettingsFrame>;
    enum class Settings {
        HEADER_TABLE_SIZE =
            0x1,  // 允许发送方以八位字节通知远程端点用于解码头块的头压缩表的最大大小
        ENABLE_PUSH = 0x2,  // 此设置可用于禁用服务器推送
        MAX_CONCURRENT_STREAMS = 0x3,  // 表示发送方允许的最大并发流数
        INITIAL_WINDOW_SIZE = 0x4,  // 表示发送方的初始窗口大小
        MAX_FRAME_SIZE = 0x5,  // 表示发送方愿意接收的最大帧有效负载的大小
        MAX_HEADER_LIST_SIZE =
            0x6  // 此通知设置以八位字节的形式通知对端，发送方准备接受的头列表的最大大小
    };

    static std::string_view SettingsToString(Settings s);

    [[nodiscard]] std::string toString() const override;
    bool writeTo(const ByteArray::ptr& ba,
                 const FrameHeader& header) const override;
    bool readFrom(const ByteArray::ptr& ba, const FrameHeader& header) override;

    std::vector<SettingsItem> items;
};

/*
 +---------------+
 |Pad Length? (8)|
 +-+-------------+-----------------------------------------------+
 |R|                  Promised Stream ID (31)                    |
 +-+-----------------------------+-------------------------------+
 |                   Header Block Fragment (*)                 ...
 +---------------------------------------------------------------+
 |                           Padding (*)                       ...
 +---------------------------------------------------------------+
*/

struct PushPromisedFrame : public IFrame {
    using ptr = std::shared_ptr<PushPromisedFrame>;
    uint8_t pad = 0;  // Flag & FrameFlagPromise::PADDED
    union {
        struct {
            uint32_t stream_id : 31;
            uint32_t r : 1;
        };
        uint32_t r_stream_id = 0;
    };
    std::string data;
    std::string padding;

    [[nodiscard]] std::string toString() const override;
    bool writeTo(const ByteArray::ptr& ba,
                 const FrameHeader& header) const override;
    bool readFrom(const ByteArray::ptr& ba, const FrameHeader& header) override;
};

/*
 +---------------------------------------------------------------+
 |                                                               |
 |                      Opaque Data (64)                         |
 |                                                               |
 +---------------------------------------------------------------+
 */

struct PingFrame : public IFrame {
    using ptr = std::shared_ptr<PingFrame>;
    static constexpr uint32_t SIZE = 8;
    union {
        uint8_t data[8];
        uint64_t uint64 = 0;
    };

    [[nodiscard]] std::string toString() const override;
    bool writeTo(const ByteArray::ptr& ba,
                 const FrameHeader& header) const override;
    bool readFrom(const ByteArray::ptr& ba, const FrameHeader& header) override;
};

/*
 +-+-------------------------------------------------------------+
 |R|                  Last-Stream-ID (31)                        |
 +-+-------------------------------------------------------------+
 |                      Error Code (32)                          |
 +---------------------------------------------------------------+
 |                  Additional Debug Data (*)                    |
 +---------------------------------------------------------------+
 */

struct GoAwayFrame : public IFrame {
    using ptr = std::shared_ptr<GoAwayFrame>;
    union {
        struct {
            uint32_t last_stream_id : 31;
            uint32_t r : 1;
        };
        uint32_t r_last_stream_id;
    };
    uint32_t error_code = 0;
    std::string data;

    [[nodiscard]] std::string toString() const override;
    bool writeTo(const ByteArray::ptr& ba,
                 const FrameHeader& header) const override;
    bool readFrom(const ByteArray::ptr& ba, const FrameHeader& header) override;
};

/*
 +-+-------------------------------------------------------------+
 |R|              Window Size Increment (31)                     |
 +-+-------------------------------------------------------------+
 */

struct WindowUpdateFrame : public IFrame {
    using ptr = std::shared_ptr<WindowUpdateFrame>;
    static constexpr uint32_t SIZE = 4;
    union {
        struct {
            uint32_t increment : 31;
            uint32_t r : 1;
        };
        uint32_t r_increment = 0;
    };

    [[nodiscard]] std::string toString() const override;
    bool writeTo(const ByteArray::ptr& ba,
                 const FrameHeader& header) const override;
    bool readFrom(const ByteArray::ptr& ba, const FrameHeader& header) override;
};

class FrameCodec {
public:
    using ptr = std::shared_ptr<FrameCodec>;

    Frame::ptr parseFrom(Stream* stream);
    size_t serializeTo(Stream* stream, const Frame::ptr& frame);
};

std::string_view FrameTypeToString(FrameType type);
std::string FrameFlagDataToString(FrameFlagData flag);
std::string FrameFlagHeadersToString(FrameFlagHeaders flag);
std::string FrameFlagSettingsToString(FrameFlagSettings flag);
std::string FrameFlagPingToString(FrameFlagPing flag);
std::string FrameFlagContinuaionToString(FrameFlagContinuation flag);
std::string FrameFlagPromiseToString(FrameFlagPromise flag);
std::string FrameFlagToString(uint8_t type, uint8_t flag);
std::string FrameRtoString(FrameR r);

#pragma pack()

}  // namespace flexy::http2