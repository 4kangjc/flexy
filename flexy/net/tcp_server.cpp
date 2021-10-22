#include "tcp_server.h"
#include "flexy/util/log.h"
#include "flexy/util/config.h"

namespace flexy {

static auto g_logger = FLEXY_LOG_NAME("system");

static auto g_tcp_server_read_timeout = Config::Lookup("tcp_server.read_timeout", 
    (uint64_t)(60 * 1000 * 2), "tcp server read timeout");


static void handleClient(const TcpServer::ptr& self, const Socket::ptr& client) {
    FLEXY_LOG_INFO(g_logger) << *client;
}

TcpServer::TcpServer(IOManager* worker, IOManager* io_worker,
        IOManager* accept_worker) : worker_(worker), ioWorker_(io_worker),
        acceptWorker_(accept_worker), recvTimeout_(g_tcp_server_read_timeout->getValue()),
        name_("flexy/1.0.0"), isStop_(true), handleClient_(handleClient) {
}

TcpServer::~TcpServer() {
    for (auto& sock : socks_) {
        sock->close();
    }
    socks_.clear();
}

bool TcpServer::bind(const Address::ptr& addr) {
    std::vector<Address::ptr> addrs;
    std::vector<Address::ptr> fails;
    addrs.push_back(addr);
    return bind(addrs, fails);
}

bool TcpServer::bind(const std::vector<Address::ptr>& addrs, std::vector<Address::ptr>& fails) {
    for (auto& addr : addrs) {
        auto sock = Socket::CreateTCP(addr->getFamily());
        if (!sock->bind(addr)) {
            FLEXY_LOG_ERROR(g_logger) << "bind fail errno = " << errno
            << " errstr = " << strerror(errno) << " addr = [" << *addr
            << "]";
            fails.push_back(addr);
            continue;
        }
        if (!sock->listen()) {
            FLEXY_LOG_ERROR(g_logger) << "listen fail errno = " << errno
            << "errstr = " << strerror(errno) << " addr = [" << *addr 
            << "]";
            fails.push_back(addr);
            continue; 
        }
        socks_.push_back(sock);
    }
    if (!fails.empty()) {
        socks_.clear();
        return false;
    }
    for (auto& sock : socks_) {
        FLEXY_LOG_INFO(g_logger) << "server bind success: " << *sock;
    }
    return true;
}

void TcpServer::startAccept(const Socket::ptr& sock) {
    while (!isStop_) {
        auto client = sock->accept();
        if (client) {
            client->setRecvTimeout(recvTimeout_);
            worker_->async(handleClient_, shared_from_this(), client);
        } else {
            FLEXY_LOG_ERROR(g_logger) << "accept errno = " << errno << "," 
            << " errstr = " << strerror(errno);
        }
    }
}

bool TcpServer::start() {
    if(!isStop_) {
        return true;
    }
    isStop_ = false;
    for (auto& sock : socks_) {
        ioWorker_->async(&TcpServer::startAccept, shared_from_this(), sock);
    }
    return true;
}

void TcpServer::stop() {
    isStop_ = true;
    acceptWorker_->async([](const TcpServer::ptr& slef){
        for (auto& sock : slef->socks_) {
            sock->cancelAll();
            sock->close();
        }
        slef->socks_.clear();
    }, shared_from_this());
}

}