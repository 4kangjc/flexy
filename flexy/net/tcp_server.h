#pragma once 

#include "flexy/util/noncopyable.h"
#include "flexy/schedule/iomanager.h"
#include "address.h"
#include "socket.h"
#include <memory>

namespace flexy {

class TcpServer;
using TcpCallBack = std::function<void
(const std::shared_ptr<TcpServer>&, const Socket::ptr&)>;

class TcpServer : public std::enable_shared_from_this<TcpServer>, noncopyable {
public:
    using ptr = std::shared_ptr<TcpServer>;
    TcpServer(IOManager* worker = IOManager::GetThis(), 
              IOManager* io_worker = IOManager::GetThis(),
              IOManager* accept_worker = IOManager::GetThis());
    virtual ~TcpServer();
    bool bind(const Address::ptr& address);
    bool bind(const std::vector<Address::ptr>& addrs, std::vector<Address::ptr>& fails);
    bool start();
    void stop();

    uint64_t getRecvTimeout() const { return recvTimeout_; }
    void setRecvTimeout(uint64_t v) { recvTimeout_ = v; }
    auto& getName() const { return name_; }
    void setName(std::string_view name) { name_ = name; }
    IOManager* getWorker() const { return worker_; }
    bool isStop() const { return isStop_; }

    void onHandleClient(TcpCallBack&& cb) { handleClient_ = std::move(cb); }
    void onHandleClient(const TcpCallBack& cb) { handleClient_ = cb; }
protected:
    void startAccept(const Socket::ptr& sock);
protected:
    std::vector<Socket::ptr> socks_;
    IOManager* worker_;
    IOManager* ioWorker_;
    IOManager* acceptWorker_;
    uint64_t recvTimeout_;
    std::string name_;
    bool isStop_;
    TcpCallBack handleClient_;

};

}