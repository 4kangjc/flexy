#include "hash_util.h"
#include "log.h"
// #include <openssl/sha.h>
#include <mbedtls/base64.h>
#include <mbedtls/md.h>
#include <mbedtls/platform.h>

namespace flexy {

static auto g_logger = FLEXY_LOG_NAME("system");

std::string base64decode(const std::string& src) {
    // int i = 0;
    // for (auto it = src.rbegin(); it != src.rend() && *it == '='; ++it, ++i);
    // size_t len = src.size() * 3 / 4 - i;
    size_t olen = 0;
    std::string ret;
    ret.resize(src.size() * 3 / 4);
    mbedtls_base64_decode((unsigned char*)ret.data(), ret.size(), &olen, (const unsigned char*)src.c_str(), src.size());
    ret.resize(olen);

    return ret;
}

std::string base64encode(const std::string& data) {
    return base64encode(data.c_str(), data.size());
}

std::string base64encode(const void* data, size_t len) {
	// static const char* base64 =
    //     "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	// std::string ret;
    // ret.reserve(len * 4 / 3 + 2);

	// const unsigned char* begin = (const unsigned char*)data;
	// const unsigned char* end   = begin + len;

    // int padding = 4;

	// while (begin < end) {
	// 	unsigned int packed = 0;
	// 	padding = std::min(end - begin, 3l);
	// 	for (int i = 0; i < padding; ++i) {
	// 		packed |= (begin[i] << (2 - i) * 8);
	// 	}
	
	// 	begin += padding;
	// 	padding = 3 - padding;
    
	// 	ret.push_back(base64[(packed >> 18) & 0x3f]);
	// 	ret.push_back(base64[(packed >> 12) & 0x3f]);
	// 	if (padding != 2) {
	// 		ret.push_back(base64[(packed >> 6) & 0x3f]);
	// 	}
	// 	if (padding == 0) {
	// 		ret.push_back(base64[(packed >> 0) & 0x3f]);
	// 	}
	// }
    // ret.append(padding, '=');

	std::string ret;
    ret.resize(len * 4 / 3 + 4);            //  必须大于olen
    size_t olen = 0;
    mbedtls_base64_encode((unsigned char*)ret.data(), ret.size(), &olen, (const unsigned char*)data, len);
    ret.resize(olen);

	return ret;
}

struct md_context {
    md_context(mbedtls_md_type_t md_type) {
        mbedtls_md_init(&ctx);
        auto info = mbedtls_md_info_from_type(md_type);
        mbedtls_md_setup(&ctx, info, 0);
        mbedtls_md_starts(&ctx);
    }
    void update(const void* data, size_t len) {
        mbedtls_md_update(&ctx, (const unsigned char*)data, len);
    }
    void update(const std::string& data) {
        mbedtls_md_update(&ctx, (const unsigned char*)data.data(), data.size());
    }
    void finish(char* output) {
        mbedtls_md_finish(&ctx, (unsigned char*)output);
    }
    ~md_context() {
        mbedtls_md_free(&ctx);
    }

    mbedtls_md_context_t ctx;
};

std::string sha1sum(const std::string& data) {
    return sha1sum(data.c_str(), data.size());
}

std::string sha1sum(const void *data, size_t len) {
    // SHA_CTX ctx;
    // SHA1_Init(&ctx);
    // SHA1_Update(&ctx, data, len);
    // std::string result;
    // result.resize(SHA_DIGEST_LENGTH);
    // SHA1_Final((unsigned char*)&result[0], &ctx);
    // return result;
    
    md_context mctx(MBEDTLS_MD_SHA1);
    mctx.update(data, len);
    std::string ret;
    ret.resize(20);
    mctx.finish(ret.data());
    return ret;
}

} // namespace flexy