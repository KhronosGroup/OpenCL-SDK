#pragma once

#if __has_include(<filesystem>)
#include <filesystem>
#elif __has_include(<experimental/filesystem>)
#include <experimental/filesystem>

namespace std {
    namespace filesystem = experimental::filesystem;
}
#else
#error "No STL filesystem support could be detected"
#endif

namespace cl::util
{
    std::filesystem::path get_exe_path();
}
