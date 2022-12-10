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
    TreeNode* left;
    TreeNode* right;
    TreeNode() : val(0), left(nullptr), right(nullptr) {}
    TreeNode(int x) : val(x), left(nullptr), right(nullptr) {}
    TreeNode(int x, TreeNode* left, TreeNode* right)
        : val(x), left(left), right(right) {}
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

    bool hasNext() { return fiber->getState() != flexy::Fiber::TERM; }

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
    s.pop_back(), s.pop_back();  // pop ", "
    s.push_back(']');
    FLEXY_LOG_INFO(g_logger) << s;  // [3, 7, 9, 15, 20]
}

int main(int argc, char** argv) {
    flexy::IOManager iom;  // fiber scheduler [1 thread]

    auto fiber_1 = flexy::fiber_make_shared(
        [](int a, int b) {  // like std::make_shared to create fiber
            FLEXY_LOG_FMT_INFO(g_logger, "{} + {} = {}", a, b,
                               a + b);  // cpp20 format log

            using namespace flexy;
            using namespace std::chrono_literals;

            FLEXY_LOG_FMT_DEBUG(
                g_logger, "fiber id = {}",
                this_fiber::get_id());  // like std::this_thread::get_id
            this_fiber::yield();        // like std::this_thread::yield
            FLEXY_LOG_DEBUG(g_logger) << "resume from hello fiber";
            // this_fiber::sleep_for(1000ms); // like std::this_thread
            // ::sleep_for
            // this_fiber::sleep_until(std::chrono::steady_clock::now() +
            // 2000ms);  // like std::this_thread::sleep_util
        },
        1, 2);
    iom.async(fiber_1);  // schedule fiber

    go[fiber_1]() {
        FLEXY_LOG_DEBUG(g_logger) << "Hello fiber";  // go style schedule lambda
        go fiber_1;                                  // resume fiber_1
    };

    run r;  // function object
    go_args(r, std::ref(argc),
            argv);  // use args [pass by reference and pass by value]

    test_tree_iterator();

    iom.async_first(
        [&argc]() { FLEXY_LOG_FMT_DEBUG(g_logger, "argc = {}", argc); });

    iom.async(
        [](int& argc) { FLEXY_LOG_FMT_DEBUG(g_logger, "argc = {}", argc); },
        std::ref(argc));
}