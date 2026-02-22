#ifndef CONFIG_CXX_CONFIG_UTILS_H
#define CONFIG_CXX_CONFIG_UTILS_H
#include <expected>
#include <filesystem>
#include <vector>

#include "config_provider.h"
#include "file_system_service.h"
namespace config::utils
{
inline auto regular_file = [](const std::filesystem::path& path) {
    return is_regular_file(path);
};

inline auto customFileOrder = [](const std::filesystem::path& path1, const std::filesystem::path& path2)
{
    const auto cxxEnv = environment::ConfigProvider::getCxxEnv();
    std::vector<std::string> order = {"default", cxxEnv, "local", "local-" + cxxEnv, "custom-environment-variables"};
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

};
inline std::expected<std::vector<std::filesystem::path>, std::string> createFilePaths(const std::filesystem::path& configFolder)
{
    const auto suppressWarning = std::getenv("SUPPRESS_NO_CONFIG_WARNING");
    if (filesystem_utils::is_directory_empty(configFolder))
    {
        const auto emptyConfigErrormessage = "No configurations found in configuration directory.";
        if (suppressWarning == nullptr)
        {
            // log this with a logger(LogLevel::Warning, "No configurations found in configuration directory.");
        }
        return std::unexpected(emptyConfigErrormessage);
    }
    std::vector<std::filesystem::path> filePaths =  std::filesystem::directory_iterator(configFolder) |
                                                    std::views::filter(regular_file) |
                                                    std::views::transform([](auto& entry){return entry.path();})|
                                                    std::ranges::to<std::vector>();
    std::ranges::sort(filePaths, customFileOrder);
    if (filePaths.empty())
    {
        return std::unexpected("There is no configuration file found.");
    }
    return filePaths;
}

}
#endif // CONFIG_CXX_CONFIG_UTILS_H
