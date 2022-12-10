#include <iostream>
#include "flexy/net/hook.h"
#include "flexy/util/log.h"
#include "iomanager.h"

namespace flexy {

struct __async__cin {
    template <typename... Args>
    static void Scan(Args&... args) {
        if (!is_hook_enable()) {
            FLEXY_LOG_INFO(FLEXY_LOG_ROOT()) << "no hook";
            return;
        }
        auto iom = IOManager::GetThis();
        if (!iom) {
            FLEXY_LOG_ERROR(FLEXY_LOG_ROOT()) << "IOManager is nullptr";
            return;
        }
        int rt = iom->onRead(0);
        if (!rt) {
            FLEXY_LOG_ERROR(FLEXY_LOG_ROOT()) << "async scanln error";
            return;
        } else {
            Fiber::Yield();
            (std::cin >> ... >> args);
        }
    }

    template <class T>
    __async__cin& operator>>(T& val) {
        if (!is_hook_enable()) {
            FLEXY_LOG_INFO(FLEXY_LOG_ROOT()) << "no hook";
            return *this;
        }
        auto iom = flexy::IOManager::GetThis();
        if (!iom) {
            FLEXY_LOG_ERROR(FLEXY_LOG_ROOT()) << "IOManager is nullptr";
            return *this;
        }
        int rt = iom->onRead(0);
        if (!rt) {
            FLEXY_LOG_ERROR(FLEXY_LOG_ROOT()) << "async cin error";
            return *this;
        } else {
            Fiber::Yield();
            std::cin >> val;
            return *this;
        }
    }

    void getline(std::string& s) {
        if (!is_hook_enable()) {
            FLEXY_LOG_INFO(FLEXY_LOG_ROOT()) << "no hook";
            return;
        }
        auto iom = flexy::IOManager::GetThis();
        if (!iom) {
            FLEXY_LOG_ERROR(FLEXY_LOG_ROOT()) << "IOManager is nullptr";
            return;
        }
        int rt = iom->onRead(0);
        if (!rt) {
            FLEXY_LOG_ERROR(FLEXY_LOG_ROOT()) << "async getline error";
            return;
        } else {
            Fiber::Yield();
            std::getline(std::cin, s);
        }
    }
};

struct __async__cout {
    template <class... Args>
    static void Print(Args&&... args) {
        if (!is_hook_enable()) {
            FLEXY_LOG_INFO(FLEXY_LOG_ROOT()) << "no hook";
            return;
        }
        auto iom = IOManager::GetThis();
        if (!iom) {
            FLEXY_LOG_ERROR(FLEXY_LOG_ROOT()) << "IOManager is nullptr";
            return;
        }
        int rt = iom->onWrite(1);
        if (!rt) {
            FLEXY_LOG_ERROR(FLEXY_LOG_ROOT()) << "async scanln error";
            return;
        } else {
            Fiber::Yield();
            ((std::cout << std::forward<Args>(args)), ...);
        }
    }

    template <class T>
    __async__cout& operator<<(T&& val) {
        if (!is_hook_enable()) {
            FLEXY_LOG_INFO(FLEXY_LOG_ROOT()) << "no hook";
            return *this;
        }
        auto iom = flexy::IOManager::GetThis();
        if (!iom) {
            FLEXY_LOG_ERROR(FLEXY_LOG_ROOT()) << "IOManager is nullptr";
            return *this;
        }
        int rt = iom->onWrite(1);
        if (!rt) {
            FLEXY_LOG_ERROR(FLEXY_LOG_ROOT()) << "async cin error";
            return *this;
        } else {
            Fiber::Yield();
            std::cout << std::forward<T>(val);
            return *this;
        }
    }

    __async__cout& operator<<(std::ostream& (*pf)(std::ostream&)) {
        pf(std::cout);
        return *this;
    }
};

extern __async__cin async_cin;
extern __async__cout async_cout;

static void getline(__async__cin& cin, std::string& s) {
    return cin.getline(s);
}

}  // namespace flexy