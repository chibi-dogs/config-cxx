#ifndef CONFIG_CXX_CONFIG_UTILS_REFACTORED_H
#define CONFIG_CXX_CONFIG_UTILS_REFACTORED_H

#include <algorithm>
#include <expected>
#include <filesystem>
#include <functional>
#include <vector>

#include "config_provider.h"
#include "file_system_service.h"

namespace config::utils
{

using FileFilter = std::function<bool(const std::filesystem::path&)>;
using FileComparator = std::function<bool(const std::filesystem::path&, const std::filesystem::path&)>;
using DirectoryReader = std::function<std::vector<std::filesystem::path>(const std::filesystem::path&)>;
using EmptyDirChecker = std::function<bool(const std::filesystem::path&)>;

inline std::vector<std::string> getFileOrder(const std::string& cxxEnv)
{
    return {"default", cxxEnv, "local", "local-" + cxxEnv, "custom-environment-variables"};
}

inline bool compareByCustomOrder(
    const std::filesystem::path& path1,
    const std::filesystem::path& path2,
    const std::vector<std::string>& order)
{
    const auto filename1 = path1.stem().string();
    const auto filename2 = path2.stem().string();

    const auto it1 = std::ranges::find(order, filename1);
    const auto it2 = std::ranges::find(order, filename2);

    if (it1 == order.end() && it2 == order.end())
    {
        // If both filenames are not in the order list, order them alphabetically
        return path1 < path2;
    }
    if (it1 == order.end())
    {
        // If only path1 is not in the order list, it comes after path2
        return false;
    }
    if (it2 == order.end())
    {
        // If only path2 is not in the order list, it comes after path1
        return true;
    }
    // If both filenames are in the order list, order them based on their position in the list
    return std::ranges::distance(order.begin(), it1) < std::ranges::distance(order.begin(), it2);
}

inline std::expected<std::vector<std::filesystem::path>, std::string> createFilePathsWithDeps(
    const std::filesystem::path& configFolder,
    const std::string& cxxEnv,
    EmptyDirChecker isEmptyChecker,
    DirectoryReader dirReader,
    FileFilter fileFilter,
    bool suppressWarning = false)
{
    if (isEmptyChecker(configFolder))
    {
        constexpr auto emptyConfigErrorMessage = "No configurations found in configuration directory.";
        if (!suppressWarning)
        {
            // log this with a logger(LogLevel::Warning, "No configurations found in configuration directory.");
        }
        return std::unexpected(emptyConfigErrorMessage);
    }

    std::vector<std::filesystem::path> filePaths = dirReader(configFolder);

    std::vector<std::filesystem::path> regularFiles;
    std::ranges::copy_if(filePaths, std::back_inserter(regularFiles), fileFilter);

    const auto order = getFileOrder(cxxEnv);
    std::ranges::sort(regularFiles, [&order](const auto& p1, const auto& p2) {
        return compareByCustomOrder(p1, p2, order);
    });

    if (regularFiles.empty())
    {
        return std::unexpected("There is no configuration file found.");
    }

    return regularFiles;
}


inline bool defaultIsDirectoryEmpty(const std::filesystem::path& p)
{
    return filesystem_utils::is_directory_empty(p);
}

inline std::vector<std::filesystem::path> defaultDirectoryReader(const std::filesystem::path& dir)
{
    std::vector<std::filesystem::path> paths;
    for (const auto& entry : std::filesystem::directory_iterator(dir))
    {
        paths.push_back(entry.path());
    }
    return paths;
}

inline bool defaultFileFilter(const std::filesystem::path& path)
{
    return is_regular_file(path);
}

inline std::expected<std::vector<std::filesystem::path>, std::string> createFilePaths(
    const std::filesystem::path& configFolder)
{
    const auto cxxEnv = environment::ConfigProvider::getCxxEnv();
    const auto suppressWarning = std::getenv("SUPPRESS_NO_CONFIG_WARNING");

    return createFilePathsWithDeps(
        configFolder,
        cxxEnv,
        defaultIsDirectoryEmpty,
        defaultDirectoryReader,
        defaultFileFilter,
        suppressWarning != nullptr
    );
}

inline auto regular_file = [](const std::filesystem::path& path) {
    return is_regular_file(path);
};

inline auto customFileOrder = [](const std::filesystem::path& path1, const std::filesystem::path& path2)
{
    const auto cxxEnv = environment::ConfigProvider::getCxxEnv();
    const auto order = getFileOrder(cxxEnv);
    return compareByCustomOrder(path1, path2, order);
};

} // namespace config::utils

#endif // CONFIG_CXX_CONFIG_UTILS_REFACTORED_H

