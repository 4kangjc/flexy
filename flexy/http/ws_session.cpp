#include "ws_session.h"
#include "flexy/util/config.h"
#include "flexy/util/hash_util.h"
#include "flexy/net/edian.h"

namespace flexy::http {

static auto g_logger = FLEXY_LOG_NAME("system");

auto g_websocket_message_max_size = Config::Lookup("websocket.message.max_size", 
                    (uint32_t) 1024 * 1024 * 32, "websocket message max size");


WSSession::WSSession(const Socket::ptr& sock, bool owner) 
                    : HttpSession(sock, owner) {}
        
HttpRequest::ptr WSSession::handleShake() {
    HttpRequest::ptr req;
    do {
        req = recvRequest();
        if (!req) {
            FLEXY_LOG_INFO(g_logger) << "invalid http request";
            break;
        }
        if (strcasecmp(req->getHeader("Upgrade").c_str(), "websocket")) {
            FLEXY_LOG_INFO(g_logger) << "http header Upgrade != websocket";
            break;
        }
        if (strcasecmp(req->getHeader("Connection").c_str(), "Upgrade")) {
            FLEXY_LOG_INFO(g_logger) << "http header Connection != Upgrade";
            break;
        }
        if (req->getHeaderAs<int>("Sec-WebSocket-Version") != 13) {
            FLEXY_LOG_INFO(g_logger) << "http header Sec-websocket-Version != 13";
        }
        std::string key = req->getHeader("Sec-WebSocket-Key");
        if (key.empty()) {
            FLEXY_LOG_INFO(g_logger) << "http header Sec-WebSocket-Key = null";
            break;
        }

        std::string v = key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
        v = base64encode(sha1sum(v));
        req->setWebsocket(true);

        auto rsp = req->createResponse();
        rsp->setStatus(HttpStatus::SWITCHING_PROTOCOLS);
        rsp->setWebsocket(true);
        rsp->setReason("Web Socket Protocol HandleShake");
        rsp->setHeader("Upgrade", "websocket");
        rsp->setHeader("Connection", "Upgrade");
        rsp->setHeader("Sec-WebSocket-Accept", v);

        sendResponse(std::move(rsp));
        FLEXY_LOG_DEBUG(g_logger) << *req;
        FLEXY_LOG_DEBUG(g_logger) << *rsp;
        return req;
    } while (false);

    if (req) {
        FLEXY_LOG_INFO(g_logger) << *req;
    }
    return nullptr;
}

std::string WSFrameHead::toString() const {
    std::stringstream ss;
    ss << "[WSFrameHead fin = " << fin << " rsv1 = "
    << rsv1 << " rsv2 = " << rsv2 << " rsv3" << rsv3
    << " opcode = " << opcode << " mask = " << mask
    << " payload = " << payload << "]";
    return ss.str();
}

WSFrameMessage::ptr WSSession::recvMessage() {
    return WSRecvMessage(this, false);
}

int32_t WSSession::sendMessage(const WSFrameMessage::ptr& msg, bool fin) {
    return WSSendMessage(this, msg, false, fin);
}

int32_t WSSession::sendMessage(std::string_view msg, int32_t opcode, bool fin) {
    return WSSendMessage(this, std::make_unique<WSFrameMessage>(opcode, msg), false, fin);
}

int32_t WSSession::ping() {
    return WSPing(this);
}

int32_t WSSession::pong() {
    return WSPong(this);
}


WSFrameMessage::ptr WSRecvMessage(Stream* stream, bool client) {
    int opcode = 0;
    std::string data;
    uint64_t cur_len = 0;
    do {
        WSFrameHead ws_head;
        if (stream->readFixSize(&ws_head, sizeof(ws_head)) <= 0) {
            break;
        }
        FLEXY_LOG_DEBUG(g_logger) << "WSFrameHead " << ws_head.toString();

        switch (ws_head.opcode) {
            case WSFrameHead::PING: {
                FLEXY_LOG_INFO(g_logger) << "PING";
                if (WSPong(stream)) {
                    goto close;
                }
                break;
            }
            case WSFrameHead::PONG :
                break;
            case WSFrameHead::CONTINUE :
            case WSFrameHead::TEXT_FRAME : 
            case WSFrameHead::BIN_FRAME : {
                if (client == ws_head.mask) {       // client ^ ws_head.mask == 1 must be true 
                    FLEXY_LOG_INFO(g_logger) << "client == ws_head.mash, "
                    "client = " << client << ", ws_head.mask = " << ws_head.mask;
                    goto close;
                }
                uint64_t length = 0;
                if (ws_head.payload == 126) {
                    uint16_t len = 0;
                    if (stream->readFixSize(&len, sizeof(len)) <= 0) {
                        goto close;
                    }
                    length = byteswap<uint16_t>(len);
                } else if (ws_head.payload == 127) {
                    uint64_t len = 0;
                    if (stream->readFixSize(&len, sizeof(len)) <= 0) {
                        goto close;
                    } 
                    length = byteswap(len);
                } else {
                    length = ws_head.payload;
                }
                
                if (cur_len + length >= g_websocket_message_max_size->getValue()) {
                    FLEXY_LOG_WARN(g_logger) << "WSFrameMessage length > "
                    << g_websocket_message_max_size->getValue() << " (" 
                    << (cur_len + length) << ")";
                    goto close;
                }

                char masking_key[4] = {0};
                if (ws_head.mask) {
                    if (stream->readFixSize(masking_key, sizeof(masking_key)) <= 0) {
                        goto close;
                    }
                }

                data.resize(cur_len + length);
                if (stream->readFixSize(&data[cur_len], length) <= 0) {
                    goto close;
                }

                if (ws_head.mask) {
                    for (uint64_t i = 0; i < length; ++i) {
                        data[cur_len + i] ^= masking_key[i % 4];
                    }
                }
                cur_len += length;

                if (!opcode && ws_head.opcode != WSFrameHead::CONTINUE) {
                    opcode = ws_head.opcode;
                }

                if (ws_head.fin) {
                    FLEXY_LOG_DEBUG(g_logger) << data;
                    return WSFrameMessage::ptr(new WSFrameMessage(opcode, std::move(data))); 
                }

                break;
            }
            default: 
                FLEXY_LOG_DEBUG(g_logger) << "invalid opcode = " << ws_head.opcode;
                break;
        }
    } while (true);
close:
    stream->close();
    return nullptr;
}

int32_t WSSendMessage(Stream* stream, const WSFrameMessage::ptr& msg, bool client, bool fin) {
    do {
        WSFrameHead ws_head;
        memset(&ws_head, 0, sizeof(ws_head));
        ws_head.fin = fin;
        ws_head.opcode = msg->getOpcode();
        ws_head.mask = client;
        uint64_t length = msg->getData().size();
        if (length < 126) {
            ws_head.payload = length;
        } else if (length < 65536) {
            ws_head.payload = 126;
        } else {
            ws_head.payload = 127;
        }

        if (stream->writeFixSize(&ws_head, sizeof(ws_head)) <= 0) {
            break;
        }

        if (ws_head.payload == 126) {
            uint16_t len = byteswap<uint16_t>(length);
            if (stream->writeFixSize(&len, sizeof(len)) <= 0) {
                break;
            }
        } else if (ws_head.payload == 127) {
            uint64_t len = byteswap(length);
            if (stream->writeFixSize(&len, sizeof(len)) <= 0) {
                break;
            }
        }

        if (client) {
            char masking_key[4];
            uint32_t rand_value = rand();
            memcpy(masking_key, &rand_value, sizeof(masking_key));
            auto& data = msg->getData();
            for (size_t i = 0; i < data.size(); ++i) {
                data[i] ^= masking_key[i % 4];
            }

            if (stream->writeFixSize(masking_key, sizeof(masking_key)) <= 0) {
                break;
            }            
        }

        if (stream->writeFixSize(msg->getData().c_str(), length) <= 0) {
            break;
        }
        return length + sizeof(ws_head);
    } while (false);
    stream->close();
    return -1;
}

int32_t WSPing(Stream* stream) {
    WSFrameHead ws_head;
    memset(&ws_head, 0, sizeof(ws_head));
    ws_head.fin = 1;
    ws_head.opcode = WSFrameHead::PING;
    int32_t v = stream->writeFixSize(&ws_head, sizeof(ws_head));
    if (v <= 0) {
        stream->close();
    }
    return v;
}

int32_t WSPong(Stream* stream) {
    WSFrameHead ws_head;
    memset(&ws_head, 0, sizeof(ws_head));
    ws_head.fin = 1;
    ws_head.opcode = WSFrameHead::PONG;
    int32_t v = stream->writeFixSize(&ws_head, sizeof(ws_head));
    if (v <= 0) {
        stream->close();
    }
    return v;
}

}