#pragma once

#include <string_view>

namespace flexy::http2::huffman {

int EncodeString(const char* in, int in_len, std::string& out, int prefix);
int EncodeString(std::string_view in, std::string& out, int prefix);
int DecodeString(const char* in, int in_len, std::string& out);
int DecodeString(std::string_view in, std::string& out);
int EncodeLen(std::string_view in);
int EncodeLen(const char* in, int in_len);

bool ShouldEncode(std::string_view in);
bool ShouldEncode(const char* in, int in_len);

}  // namespace flexy::http2::huffman