#pragma once

#include <fstream>
#include <string>

namespace cl
{
namespace util
{
    std::string read_text_file(const char * filename);
}
}

std::string cl::util::read_text_file(const char * filename)
{
    std::ifstream in(filename);
    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}
