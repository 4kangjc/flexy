#include "flexy/util/hash_util.h"
#include "flexy/util/log.h"
#include "flexy/util/macro.h"

static auto& g_logger = FLEXY_LOG_ROOT();

void test_base64() {
    for (int i = 0; i < 100000; ++i) {
        int len = rand() % 100;
        std::string src;
        src.resize(len);
        for (int j = 0; j < len; ++j) {
            src[j] = rand() % 255;
        }
        auto rst = flexy::base64encode(src);
        // FLEXY_LOG_INFO(g_logger) << rst;
        auto ret = flexy::base64decode(rst);
        FLEXY_ASSERT2(ret == src, ret << " " << src
                                      << " , rst.size() == " << ret.size()
                                      << ", src.size()" << src.size());
    }
}

std::string test_sha1(const std::string& key) {
    std::string v = key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    return flexy::base64encode(flexy::sha1sum(v));
}

int main(int argc, char** argv) {
    test_base64();
    if (argc > 1) {
        FLEXY_LOG_DEBUG(g_logger) << test_sha1(argv[1]);
    }
}