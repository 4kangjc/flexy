#pragma once

#include <string>

namespace flexy {

std::string base64decode(const std::string& src);
std::string base64encode(const std::string& data);
std::string base64encode(const void* data, size_t len);

std::string sha1sum(const std::string& data);
std::string sha1sum(const void* data, size_t len);

}  // namespace flexy