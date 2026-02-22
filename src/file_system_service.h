#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace config::filesystem_utils
{
    std::string read(const std::filesystem::path& absolutePath);
    [[nodiscard]] bool exists(const std::filesystem::path& absolutePath);
    [[nodiscard]] bool isDirectory(const std::filesystem::path& absolutePath);
    [[nodiscard]] bool isRelative(const std::filesystem::path& path);
    std::filesystem::path getSystemRootPath();
    std::filesystem::path getCurrentWorkingDirectory();
    std::filesystem::path getExecutablePath();
    bool is_directory_empty(const std::filesystem::path& p);
    };
