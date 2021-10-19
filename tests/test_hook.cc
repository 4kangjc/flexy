#include <flexy/net/hook.h>
#include <flexy/util/log.h>
#include <flexy/schedule/iomanager.h>

#include <sys/socket.h>
#include <arpa/inet.h>

static auto&& g_logger = FLEXY_LOG_ROOT();

void test_sleep() {
    flexy::IOManager iom(1);
    iom.async([](){
        sleep(2);
        FLEXY_LOG_INFO(g_logger) << "sleep 2";
    });
    iom.async([](){
        sleep(3);
        FLEXY_LOG_INFO(g_logger) << "sleep 3";
    });
    FLEXY_LOG_INFO(g_logger) << "test_sleep";
}

void test_sock() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "36.152.44.96", &addr.sin_addr);

    FLEXY_LOG_INFO(g_logger) << "begin connect";
    int rt = connect(sock, (const sockaddr*)&addr, sizeof(addr));
    FLEXY_LOG_INFO(g_logger) << "socket = " << sock << ", connect rt = " << rt << " errno = " << errno;
    if (rt) {
        return;
    }
    const char data[] = "GET / HTTP/1.0\r\n\r\n";
    rt = send(sock, data, sizeof(data), 0);
    FLEXY_LOG_INFO(g_logger) << "send rt = " << rt << " errno = " << errno;

    if (rt <= 0) {
        return;
    }
    std::string buff;
    buff.resize(4096);
    rt = recv(sock, &buff[0], buff.size(), 0);
    FLEXY_LOG_INFO(g_logger) << "recv rt = " << rt << " errno = " << errno;
    if (rt <= 0) {
        return;
    }
    buff.resize(rt);
    FLEXY_LOG_INFO(g_logger) << buff;
}

int main() {
    // test_sleep();
    flexy::IOManager iom;
    iom.async(test_sock);
}