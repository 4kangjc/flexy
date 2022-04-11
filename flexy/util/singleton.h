#pragma once

#include <memory>

namespace flexy {

template <typename T>
class Singleton {
public:
    static T& GetInstance() {
        static T v;
        return v;
    }
};

template <typename T>
class SingletonPtr {
public: 
    static std::shared_ptr<T> GetInstance() {
        static auto v = std::make_shared<T>();
        return v;
    }
};

}