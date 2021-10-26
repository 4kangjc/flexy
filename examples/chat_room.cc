#include <flexy/flexy.h>

using namespace flexy;

struct ChatRoom : public TcpServer {
public:
    void handleClient(const Socket::ptr& client) override {
        {
            WRITELOCK(mutex_);
            clients_.insert(client);
        }
        while (!isStop_) {
            std::string s;
            s.resize(1024);
            if (client->recv(s) <= 0) {
                {
                    WRITELOCK(mutex_);
                    clients_.erase(client);
                }
                break;
            }
            READLOCK(mutex_);
            for (auto& sock : clients_) {
                if (sock != client) {
                    sock->send(s);
                }
            }
        }
    }
private:
    std::set<Socket::ptr> clients_;
    rw_mutex mutex_;
};

void run() {
    auto cr = std::make_shared<ChatRoom>();
    auto addr = Address::LookupAny("0.0.0.0:8021");
    cr->bind(addr);
    cr->start();
    Signal::signal(SIGINT, [cr](){ cr->stop(); });
}

int main() {
    IOManager iom(2);
    iom.async(run);
}