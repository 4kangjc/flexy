#include <flexy/flexy.h>

using namespace flexy;

struct ChatRoom : public TcpServer {
    std::set<Socket::ptr> clients_;
    rw_mutex mutex_;
};

void run() {
    auto cr = std::make_shared<ChatRoom>();
    cr->onHandleClient([](const TcpServer::ptr& self, const Socket::ptr& client) {
        auto thi = std::dynamic_pointer_cast<ChatRoom>(self);
        {
            WRITELOCK(thi->mutex_);
            thi->clients_.insert(client);
        }
        while (!self->isStop()) {
            std::string s;
            s.resize(1024);
            if (client->recv(s) <= 0) {
                {
                    WRITELOCK(thi->mutex_);
                    thi->clients_.erase(client);
                }
                break;
            }
            READLOCK(thi->mutex_);
            for (auto& sock : thi->clients_) {
                if (sock != client) {
                    sock->send(s);
                }
            }
        }
    });
    auto addr = Address::LookupAny("0.0.0.0:8021");
    cr->bind(addr);
    cr->start();
    Signal::signal(SIGINT, [cr](){ cr->stop(); });
}

int main() {
    IOManager iom(2);
    iom.async(run);
}