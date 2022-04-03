# flexy 
## 介绍
一个高性能的C++服务器框架，采用C++17标准，基于协程，实现以同步的方式编写异步代码，配置加载可使用json或yaml，支持IPv6，支持TCP/UDP，简单实现了HTTP/1.1，WebSocket协议

## 特点
* 现代C++设计风格, 广泛使用了 C++11/14/17的新语法特性和标准库
* 采用IO多路复用与协程配合，实现了与具体业务解耦，方便业务开发人员以便利的同步调用语法编写高性能的异步调用代码
* N-M的协程调度模型，N个线程，M个协程，N个线程调度M个协程
* 统一事件源，统一处理信号事件，IO事件，定时事件
* 提供一系列基础库, 比如日志，配置等

## 编译
* 编译环境  
`gcc version 11.1.0   (最低 gcc 7.1)`   
`cmake version 3.21.1 (最低 cmake 3.8)`  
`boost version 1.76 `  
`ragel version 6.10`  

* 编译依赖   
`sudo pacman -S boost yaml-cpp jsoncpp fmt ragel mbedtls mysql sqlite`  

* 编译安装
```shell
git clone https://github.com/4kangjc/flexy.git
cd flexy
mkdir build && cd build && cmake ..
make -j
```

## 致谢
* [sylar](https://github.com/sylar-yin/sylar)
* [handy](https://github.com/yedf2/handy)
