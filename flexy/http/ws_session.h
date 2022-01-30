#pragma once

#include <stdint.h>

#include "http_session.h"

namespace flexy::http {

#pragma pack(1)
struct WSFrameHead {
    enum OPCODE {
        // 数据分片帧
        CONTINUE = 0,
        // 文本帧
        TEXT_FRAME = 1,
        // 二进制帧
        BIN_FRAME = 2,
        // 断开连接
        CLOSE = 8,
        // PING
        PING = 9,
        // PONG
        PONG = 0xA
    };
    uint32_t opcode : 4;
    bool rsv3 : 1;
    bool rsv2 : 1;
    bool rsv1 : 1;
    bool fin  : 1;
    uint32_t payload : 7;
    bool mask : 1;

    std::string toString() const;
    operator std::string() { return toString(); }
};
#pragma pack()


class WSFrameMessage {
public:
    using ptr = std::unique_ptr<WSFrameMessage>;
    WSFrameMessage(int opcode = 0, std::string_view data = "") 
        : opcode_(opcode), data_(data) { }
    WSFrameMessage(int opcode = 0, std::string&& data = "") 
        : opcode_(opcode), data_(std::move(data)) { } 

    int getOpcode() const { return opcode_; }
    void setOpcode(int v) { opcode_ = v; }

    auto& getData() const { return data_; }
    auto& getData() { return data_; }
    void setData(std::string_view v) { data_ = v; } 
private:
    int opcode_;
    std::string data_;
};

class WSSession : public HttpSession {
public:
    using ptr = std::unique_ptr<WSSession>;
    WSSession(const Socket::ptr& sock, bool owner = true);
    
    HttpRequest::ptr handleShake();

    WSFrameMessage::ptr recvMessage();
    int32_t sendMessage(const WSFrameMessage::ptr& msg, bool fin = true);
    int32_t sendMessage(std::string_view msg, int32_t opcode = WSFrameHead::TEXT_FRAME, bool fin = true);
    int32_t ping();
    int32_t pong();
private:
    bool handleServerShake();
    bool handleClientShake();
};

WSFrameMessage::ptr WSRecvMessage(Stream* stream, bool client);
int32_t WSSendMessage(Stream* stream, const WSFrameMessage::ptr& msg, bool client, bool fin);
int32_t WSPing(Stream* stream);
int32_t WSPong(Stream* stream);

}