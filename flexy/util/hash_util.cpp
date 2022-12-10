#include "hash_util.h"
#include <openssl/sha.h>
#include "log.h"
// #include <mbedtls/base64.h>
// #include <mbedtls/md.h>
// #include <mbedtls/platform.h>

namespace flexy {

static auto g_logger = FLEXY_LOG_NAME("system");

static inline bool is_base64(char c) {
    return (isalnum(c) || (c == '+') || (c == '/'));
}

static const std::string base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

std::string base64decode(const std::string& encoded_string) {
    // size_t olen = 0;
    // std::string ret;
    // ret.resize(src.size() * 3 / 4);
    // mbedtls_base64_decode((unsigned char*)ret.data(), ret.size(), &olen,
    // (const unsigned char*)src.c_str(), src.size()); ret.resize(olen);

    int in_len = encoded_string.size();
    int i = 0;
    int j = 0;
    int in_ = 0;
    char char_array_4[4], char_array_3[3];
    std::string ret;

    while (in_len-- && (encoded_string[in_] != '=') &&
           is_base64(encoded_string[in_])) {
        char_array_4[i++] = encoded_string[in_];
        in_++;
        if (i == 4) {
            for (i = 0; i < 4; i++)
                char_array_4[i] = base64_chars.find(char_array_4[i]);

            char_array_3[0] =
                (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) +
                              ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (i = 0; (i < 3); i++)
                ret += char_array_3[i];
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 4; j++)
            char_array_4[j] = 0;

        for (j = 0; j < 4; j++)
            char_array_4[j] = base64_chars.find(char_array_4[j]);

        char_array_3[0] =
            (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] =
            ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

        for (j = 0; (j < i - 1); j++)
            ret += char_array_3[j];
    }

    return ret;
}

std::string base64encode(const std::string& data) {
    return base64encode(data.c_str(), data.size());
}

std::string base64encode(const void* data, size_t len) {
    std::string ret;
    ret.reserve(len * 4 / 3 + 2);

    const unsigned char* begin = (const unsigned char*)data;
    const unsigned char* end = begin + len;

    int padding = 4;

    while (begin < end) {
        unsigned int packed = 0;
        padding = std::min(end - begin, 3l);
        for (int i = 0; i < padding; ++i) {
            packed |= (begin[i] << (2 - i) * 8);
        }

        begin += padding;
        padding = 3 - padding;

        ret.push_back(base64_chars[(packed >> 18) & 0x3f]);
        ret.push_back(base64_chars[(packed >> 12) & 0x3f]);
        if (padding != 2) {
            ret.push_back(base64_chars[(packed >> 6) & 0x3f]);
        }
        if (padding == 0) {
            ret.push_back(base64_chars[(packed >> 0) & 0x3f]);
        }
    }
    ret.append(padding, '=');

    // std::string ret;
    // ret.resize(len * 4 / 3 + 4);            //  必须大于olen
    // size_t olen = 0;
    // mbedtls_base64_encode((unsigned char*)ret.data(), ret.size(), &olen,
    // (const unsigned char*)data, len); ret.resize(olen);

    return ret;
}

// struct md_context {
//     md_context(mbedtls_md_type_t md_type) {
//         mbedtls_md_init(&ctx);
//         auto info = mbedtls_md_info_from_type(md_type);
//         mbedtls_md_setup(&ctx, info, 0);
//         mbedtls_md_starts(&ctx);
//     }
//     void update(const void* data, size_t len) {
//         mbedtls_md_update(&ctx, (const unsigned char*)data, len);
//     }
//     void update(const std::string& data) {
//         mbedtls_md_update(&ctx, (const unsigned char*)data.data(),
//         data.size());
//     }
//     void finish(char* output) {
//         mbedtls_md_finish(&ctx, (unsigned char*)output);
//     }
//     ~md_context() {
//         mbedtls_md_free(&ctx);
//     }

//     mbedtls_md_context_t ctx;
// };

std::string sha1sum(const std::string& data) {
    return sha1sum(data.c_str(), data.size());
}

std::string sha1sum(const void* data, size_t len) {
    SHA_CTX ctx;
    SHA1_Init(&ctx);
    SHA1_Update(&ctx, data, len);
    std::string result;
    result.resize(SHA_DIGEST_LENGTH);
    SHA1_Final((unsigned char*)&result[0], &ctx);
    return result;

    // md_context mctx(MBEDTLS_MD_SHA1);
    // mctx.update(data, len);
    // std::string ret;
    // ret.resize(20);
    // mctx.finish(ret.data());
    // return ret;
}

}  // namespace flexy