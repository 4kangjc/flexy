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
`sudo pacman -S boost yaml-cpp jsoncpp fmt openssl ragel mysql sqlite gtest`  

* 编译安装
```shell
git clone https://github.com/4kangjc/flexy.git
cd flexy
mkdir build && cd build && cmake ..
make -j
```

```shell 
# 测试
# sudo pacman -S gtest  
cmake -DTESTS=on ..
make -j
```

## 示例代码[协程]
```cpp
#include <flexy/flexy.h>

static auto& g_logger = FLEXY_LOG_ROOT();

struct run {
    void operator()(int& argc, char** argv) {
        for (int i = 1; i < argc; ++i) {
            FLEXY_LOG_DEBUG(g_logger) << argv[i];
        }
        argc = 0x99;
    }
};

struct TreeNode {
    int val;
    TreeNode *left;
    TreeNode *right;
    TreeNode() : val(0), left(nullptr), right(nullptr) {}
    TreeNode(int x) : val(x), left(nullptr), right(nullptr) {}
    TreeNode(int x, TreeNode *left, TreeNode *right) : val(x), left(left), right(right) {}
};
 

// leetcode 173
struct BSTIterator {
private:
    void dfs(TreeNode* root) {
        if (root == nullptr) {
            return;
        }
        dfs(root->left);
        cur = root->val;
        flexy::this_fiber::yield();
        dfs(root->right);
    }
public:
    BSTIterator(TreeNode* root) {
        fiber = flexy::fiber_make_shared(&BSTIterator::dfs, this, root);
    }
    
    int next() {
        fiber->resume();
        return cur;
    }
    
    bool hasNext() {
        return fiber->getState() != flexy::Fiber::TERM;
    }
private:
    int cur;
    flexy::Fiber::ptr fiber;
};

void test_tree_iterator() {
    TreeNode node_3(9), node_4(20);
    TreeNode node_1(3), node_2(15, &node_3, &node_4);
    TreeNode root(7, &node_1, &node_2);

    BSTIterator tree_iterator(&root);
    
    std::stringstream ss;
    ss << '[';
    while (tree_iterator.hasNext()) {
        ss << tree_iterator.next() << ", ";
    }
    std::string s(std::move(ss.str()));
    s.pop_back(), s.pop_back();                                                 // pop ", "
    s.push_back(']');
    FLEXY_LOG_INFO(g_logger) << s;                                              // [3, 7, 9, 15, 20]
}

int main(int argc, char** argv) {
    flexy::IOManager iom;                                                       // fiber scheduler [1 thread]

    auto fiber_1 = flexy::fiber_make_shared([] (int a, int b) {                 // like std::make_shared to create fiber
        FLEXY_LOG_FMT_INFO(g_logger, "{} + {} = {}", a, b, a + b);              // cpp20 format log
        
        using namespace flexy;
        using namespace std::chrono_literals;
        
        FLEXY_LOG_FMT_DEBUG(g_logger, "fiber id = {}", this_fiber::get_id());   // like std::this_thread::get_id
        this_fiber::yield();                                                    // like std::this_thread::yield
        FLEXY_LOG_DEBUG(g_logger) << "resume from hello fiber";         
        // this_fiber::sleep_for(1000ms);                                       // like std::this_thread ::sleep_for
        // this_fiber::sleep_until(std::chrono::steady_clock::now() + 2000ms);  // like std::this_thread::sleep_util
    }, 1, 2);
    iom.async(fiber_1);                                                         // schedule fiber

    go [fiber_1]() {
        FLEXY_LOG_DEBUG(g_logger) << "Hello fiber";                             // go style schedule lambda
        go fiber_1;                                                             // resume fiber_1
    };

    run r;                                                                      // function object
    go_args(r, std::ref(argc), argv);                                           // use args [pass by reference and pass by value]

    test_tree_iterator();

    iom.async_first([&argc](){
        FLEXY_LOG_FMT_DEBUG(g_logger, "argc = {}", argc);
    });

    iom.async([](int& argc){
        FLEXY_LOG_FMT_DEBUG(g_logger, "argc = {}", argc);
    }, std::ref(argc));
}
```

## TODO
- [http2](https://github.com/4kangjc/flexy/tree/dev/flexy/http2) 
- tls **[mbedtls/openssl]**
- rpc **[protobuf/json]**
- [gdb调试协程插件](https://github.com/Tencent/flare/blob/master/flare/doc/gdb-plugin.md)
- [http3]


## 致谢
* [sylar](https://github.com/sylar-yin/sylar)
* [flare](https://github.com/Tencent/flare)
* [handy](https://github.com/yedf2/handy)
