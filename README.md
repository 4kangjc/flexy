# flexy 
## 介绍
一个高性能的C++服务器框架，采用C++17标准，基于协程，实现以同步的方式编写异步代码，配置加载可使用json或yaml
## 编译
* 编译环境
`gcc version 11.1.0   (最低 gcc 7.1)`   
`cmake version 3.21.1 (最低 cmake 3.8)`  
`boost version 1.76 `  
`ragel version 6.10`  

* 编译依赖
1. boost  
**ArchLinux** : `sudo pacman -S boost`  
**Ubuntu**    : `sudo apt install libboost-all-dev`  
**Centos**    :  `sudo yum install boost-devel`
2. yaml-cpp  
**ArchLinux** : `sudo pacman -S yaml-cpp`
```
git clone https://github.com/jbeder/yaml-cpp.git
cd yaml-cpp
mkdir build && cd build
cmake -DBUILD_SHARED_LIBS=ON .. //动态库, 默认为静态库(cmake ..)
make && make install
```
3. jsoncpp  
**ArchLinux** : `sudo pacman -S jsoncpp`
4. fmt  
**ArchLinux** : `sudo pacman -S fmt`
```
git clone  https://github.com/fmtlib/fmt.git
cd fmt
mkdir build && cd build && cmake ..
make && make install
```
5. ragel  
**ArchLinux** : `sudo pacman -S ragel`  
**Ubuntu**    : `sudo apt install ragel`  
**Centos**    : `sudo yum install ragel`
* 编译安装
```shell
git clone https://github.com/4kangjc/flexy.git
cd flexy
mkdir build && cd build && cmake ..
make -j
```