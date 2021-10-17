#include "flexy/schedule/iomanager.h"
#include "flexy/util/macro.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <sys/epoll.h>


static auto&& g_logger = FLEXY_LOG_ROOT();

int sock = 0;
 

void test_fiber() {
    sock = socket(AF_INET, SOCK_STREAM, 0);
    FLEXY_ASSERT(sock >= 0);
    FLEXY_LOG_INFO(g_logger) << "test_fiber sock = " << sock;
    fcntl(sock, F_SETFL, O_NONBLOCK);
    sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "36.152.44.95", &addr.sin_addr);   // 百度ip会更换
    if (!connect(sock, (sockaddr*)&addr, sizeof(addr))) {

    } else if (errno == EINPROGRESS) {
        FLEXY_LOG_INFO(g_logger) << "add event error = " << errno << " " << strerror(errno);
        flexy::IOManager::GetThis()->addEvent(sock, flexy::READ, [](){
            FLEXY_LOG_INFO(g_logger) << "read call back";
        }); 
        flexy::IOManager::GetThis()->addEvent(sock, flexy::WRITE, [](){
            FLEXY_LOG_INFO(g_logger) << "write call back";
            //FLEXY::IOManager::GetThis()->addEvent(sock, FLEXY::IOManager::READ, [](){ FLEXY_LOG_DEBUG(g_logger) << "read"; } );
            bool res = flexy::IOManager::GetThis()->cancelEvent(sock, flexy::READ);
            if (!res) {
                FLEXY_LOG_INFO(g_logger) << "cacel event failed";
            }
            close(sock);
        });
        //FLEXY::IOManager::GetThis()->addEvent(sock, FLEXY::IOManager::WRITE, [](){ FLEXY_LOG_DEBUG(g_logger) << "1"; } );
        //FLEXY::IOManager::GetThis()->addEvent(sock, FLEXY::IOManager::READ, [](){ FLEXY_LOG_DEBUG(g_logger) << "read"; } );
        //FLEXY::IOManager::GetThis()->cancelEvent(sock, FLEXY::IOManager::READ);
        //FLEXY::IOManager::GetThis()->delEvent(sock, FLEXY::IOManager::WRITE);
        //FLEXY::IOManager::GetThis()->addEvent(sock, FLEXY::IOManager::WRITE, [](){ FLEXY_LOG_DEBUG(g_logger) << "2"; } );
        //FLEXY::IOManager::GetThis()->delEvent(sock, FLEXY::IOManager::WRITE);
        //FLEXY::IOManager::GetThis()->addEvent(sock, FLEXY::IOManager::WRITE, [](){ FLEXY_LOG_DEBUG(g_logger) << "3"; } );
    } else {
        FLEXY_LOG_INFO(g_logger) << "else " << errno << " " << strerror(errno);
    }
}

void test1() {
    flexy::IOManager iom;
    //FLEXY::IOManager iom(2, false);
    iom.async(test_fiber);
}


int main() {
    test1();
    return 0;
}