#include "config_utils_refactored.h"

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "gtest/gtest.h"

#include "environment_setter.h"
#include "file_system_service.h"

using namespace ::testing;
using namespace config::utils;
using namespace config::tests;
using namespace config::filesystem_utils;

namespace
{
const auto projectRootPath = getExecutablePath();
const auto testConfigDirectory = projectRootPath.parent_path() / "configUtilsTest";
}

class ConfigUtilsTest : public Test
{
public:
    void SetUp() override
    {
        std::filesystem::remove_all(testConfigDirectory);
        std::filesystem::create_directory(testConfigDirectory);
    }

    void TearDown() override
    {
        std::filesystem::remove_all(testConfigDirectory);
    }
};

// ==================== PURE FUNCTION TESTS ====================
// These tests don't require filesystem or environment setup

TEST_F(ConfigUtilsTest, getFileOrder_withDevelopmentEnv_returnsCorrectOrder)
{
    const auto order = getFileOrder("development");

    const std::vector<std::string> expected = {"default", "development", "local", "local-development", "custom-environment-variables"};

    ASSERT_EQ(order, expected);
}

TEST_F(ConfigUtilsTest, getFileOrder_withProductionEnv_returnsCorrectOrder)
{
    const auto order = getFileOrder("production");

    std::vector<std::string> expected = {"default", "production", "local", "local-production", "custom-environment-variables"};

    ASSERT_EQ(order, expected);
}

TEST_F(ConfigUtilsTest, getFileOrder_withTestEnv_returnsCorrectOrder)
{
    const auto order = getFileOrder("test");

    std::vector<std::string> expected = {"default", "test", "local", "local-test", "custom-environment-variables"};

    ASSERT_EQ(order, expected);
}

TEST_F(ConfigUtilsTest, compareByCustomOrder_bothInOrder_sortsCorrectly)
{
    const std::vector<std::string> order = {"default", "development", "local", "local-development", "custom-environment-variables"};

    std::filesystem::path default_json = "default.json";
    std::filesystem::path development_json = "development.json";

    ASSERT_TRUE(compareByCustomOrder(default_json, development_json, order));
    ASSERT_FALSE(compareByCustomOrder(development_json, default_json, order));
}

TEST_F(ConfigUtilsTest, compareByCustomOrder_oneNotInOrder_unknownComesLast)
{
    const std::vector<std::string> order = {"default", "development", "local"};

    std::filesystem::path default_json = "default.json";
    std::filesystem::path unknown_json = "unknown.json";

    // default is in order, unknown is not -> default comes first
    ASSERT_TRUE(compareByCustomOrder(default_json, unknown_json, order));
    ASSERT_FALSE(compareByCustomOrder(unknown_json, default_json, order));
}

TEST_F(ConfigUtilsTest, compareByCustomOrder_bothNotInOrder_sortsAlphabetically)
{
    const std::vector<std::string> order = {"default", "development"};

    std::filesystem::path abc_json = "abc.json";
    std::filesystem::path xyz_json = "xyz.json";

    ASSERT_TRUE(compareByCustomOrder(abc_json, xyz_json, order));
    ASSERT_FALSE(compareByCustomOrder(xyz_json, abc_json, order));
}

TEST_F(ConfigUtilsTest, compareByCustomOrder_customEnvironmentVariablesComesLast)
{
    const std::vector<std::string> order = {"default", "development", "local", "local-development", "custom-environment-variables"};

    std::filesystem::path local_dev = "local-development.json";
    std::filesystem::path custom_env = "custom-environment-variables.json";

    ASSERT_TRUE(compareByCustomOrder(local_dev, custom_env, order));
    ASSERT_FALSE(compareByCustomOrder(custom_env, local_dev, order));
}

// ==================== DEPENDENCY INJECTION TESTS ====================
// These tests use mocked dependencies to test business logic without filesystem

TEST_F(ConfigUtilsTest, createFilePathsWithDeps_emptyDirectory_returnsError)
{
    auto mockIsEmpty = [](const std::filesystem::path&) { return true; };
    auto mockReader = [](const std::filesystem::path&) { return std::vector<std::filesystem::path>{}; };
    auto mockFilter = [](const std::filesystem::path&) { return true; };

    const auto result = createFilePathsWithDeps(
        "/fake/path",
        "development",
        mockIsEmpty,
        mockReader,
        mockFilter,
        false
    );

    ASSERT_FALSE(result.has_value());
    ASSERT_EQ(result.error(), "No configurations found in configuration directory.");
}

TEST_F(ConfigUtilsTest, createFilePathsWithDeps_emptyDirectoryWithSuppressWarning_returnsError)
{
    auto mockIsEmpty = [](const std::filesystem::path&) { return true; };
    auto mockReader = [](const std::filesystem::path&) { return std::vector<std::filesystem::path>{}; };
    auto mockFilter = [](const std::filesystem::path&) { return true; };

    const auto result = createFilePathsWithDeps(
        "/fake/path",
        "development",
        mockIsEmpty,
        mockReader,
        mockFilter,
        true  // suppress warning
    );

    ASSERT_FALSE(result.has_value());
    ASSERT_EQ(result.error(), "No configurations found in configuration directory.");
}

TEST_F(ConfigUtilsTest, createFilePathsWithDeps_noRegularFiles_returnsError)
{
    auto mockIsEmpty = [](const std::filesystem::path&) { return false; };
    auto mockReader = [](const std::filesystem::path&) {
        return std::vector<std::filesystem::path>{"/fake/dir1", "/fake/dir2"};
    };
    auto mockFilter = [](const std::filesystem::path&) { return false; }; // No regular files

    const auto result = createFilePathsWithDeps(
        "/fake/path",
        "development",
        mockIsEmpty,
        mockReader,
        mockFilter,
        false
    );

    ASSERT_FALSE(result.has_value());
    ASSERT_EQ(result.error(), "There is no configuration file found.");
}

TEST_F(ConfigUtilsTest, createFilePathsWithDeps_singleFile_returnsFile)
{
    auto mockIsEmpty = [](const std::filesystem::path&) { return false; };
    auto mockReader = [](const std::filesystem::path&) {
        return std::vector<std::filesystem::path>{"/fake/default.json"};
    };
    auto mockFilter = [](const std::filesystem::path&) { return true; };

    const auto result = createFilePathsWithDeps(
        "/fake/path",
        "development",
        mockIsEmpty,
        mockReader,
        mockFilter,
        false
    );

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 1);
    ASSERT_EQ(result->at(0), "/fake/default.json");
}

TEST_F(ConfigUtilsTest, createFilePathsWithDeps_multipleFiles_sortedCorrectly)
{
    auto mockIsEmpty = [](const std::filesystem::path&) { return false; };
    auto mockReader = [](const std::filesystem::path&) {
        return std::vector<std::filesystem::path>{
            "/fake/custom-environment-variables.json",
            "/fake/local.json",
            "/fake/development.json",
            "/fake/default.json"
        };
    };
    auto mockFilter = [](const std::filesystem::path&) { return true; };

    const auto result = createFilePathsWithDeps(
        "/fake/path",
        "development",
        mockIsEmpty,
        mockReader,
        mockFilter,
        false
    );

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 4);

    // Verify correct order
    ASSERT_EQ(result->at(0).filename(), "default.json");
    ASSERT_EQ(result->at(1).filename(), "development.json");
    ASSERT_EQ(result->at(2).filename(), "local.json");
    ASSERT_EQ(result->at(3).filename(), "custom-environment-variables.json");
}

TEST_F(ConfigUtilsTest, createFilePathsWithDeps_withLocalEnvFile_sortsCorrectly)
{
    auto mockIsEmpty = [](const std::filesystem::path&) { return false; };
    auto mockReader = [](const std::filesystem::path&) {
        return std::vector<std::filesystem::path>{
            "/fake/custom-environment-variables.json",
            "/fake/local-production.json",
            "/fake/local.json",
            "/fake/production.json",
            "/fake/default.json"
        };
    };
    auto mockFilter = [](const std::filesystem::path&) { return true; };

    const auto result = createFilePathsWithDeps(
        "/fake/path",
        "production",
        mockIsEmpty,
        mockReader,
        mockFilter,
        false
    );

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 5);

    // Verify correct order for production environment
    ASSERT_EQ(result->at(0).filename(), "default.json");
    ASSERT_EQ(result->at(1).filename(), "production.json");
    ASSERT_EQ(result->at(2).filename(), "local.json");
    ASSERT_EQ(result->at(3).filename(), "local-production.json");
    ASSERT_EQ(result->at(4).filename(), "custom-environment-variables.json");
}

TEST_F(ConfigUtilsTest, createFilePathsWithDeps_withUnknownFiles_unknownFilesSortedAlphabetically)
{
    auto mockIsEmpty = [](const std::filesystem::path&) { return false; };
    auto mockReader = [](const std::filesystem::path&) {
        return std::vector<std::filesystem::path>{
            "/fake/custom-environment-variables.json",
            "/fake/zebra.json",
            "/fake/apple.json",
            "/fake/default.json"
        };
    };
    auto mockFilter = [](const std::filesystem::path&) { return true; };

    const auto result = createFilePathsWithDeps(
        "/fake/path",
        "development",
        mockIsEmpty,
        mockReader,
        mockFilter,
        false
    );

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 4);

    // Known files come first in order, unknown files sorted alphabetically at the end
    ASSERT_EQ(result->at(0).filename(), "default.json");
    ASSERT_EQ(result->at(1).filename(), "custom-environment-variables.json");
    ASSERT_EQ(result->at(2).filename(), "apple.json");
    ASSERT_EQ(result->at(3).filename(), "zebra.json");
}

TEST_F(ConfigUtilsTest, createFilePathsWithDeps_mixedFilesAndDirectories_onlyFilesReturned)
{
    auto mockIsEmpty = [](const std::filesystem::path&) { return false; };
    auto mockReader = [](const std::filesystem::path&) {
        return std::vector<std::filesystem::path>{
            "/fake/default.json",
            "/fake/somedir",
            "/fake/development.json",
            "/fake/anotherdir"
        };
    };
    auto mockFilter = [](const std::filesystem::path& p) {
        // Only json files are regular files
        return p.extension() == ".json";
    };

    const auto result = createFilePathsWithDeps(
        "/fake/path",
        "development",
        mockIsEmpty,
        mockReader,
        mockFilter,
        false
    );

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 2);
    ASSERT_EQ(result->at(0).filename(), "default.json");
    ASSERT_EQ(result->at(1).filename(), "development.json");
}

// ==================== INTEGRATION TESTS ====================
// These tests use real filesystem

TEST_F(ConfigUtilsTest, createFilePaths_withRealFilesystem_worksCorrectly)
{
    EnvironmentSetter::setEnvironmentVariable("CXX_ENV", "development");

    // Create test files
    std::ofstream{testConfigDirectory / "default.json"} << "{}";
    std::ofstream{testConfigDirectory / "development.json"} << "{}";
    std::ofstream{testConfigDirectory / "local.json"} << "{}";

    const auto result = createFilePaths(testConfigDirectory);

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 3);
    ASSERT_EQ(result->at(0).filename(), "default.json");
    ASSERT_EQ(result->at(1).filename(), "development.json");
    ASSERT_EQ(result->at(2).filename(), "local.json");
}

TEST_F(ConfigUtilsTest, createFilePaths_emptyRealDirectory_returnsError)
{
    const auto result = createFilePaths(testConfigDirectory);

    ASSERT_FALSE(result.has_value());
    ASSERT_EQ(result.error(), "No configurations found in configuration directory.");
}

TEST_F(ConfigUtilsTest, createFilePaths_withOnlyDirectories_returnsError)
{
    std::filesystem::create_directory(testConfigDirectory / "subdir1");
    std::filesystem::create_directory(testConfigDirectory / "subdir2");

    const auto result = createFilePaths(testConfigDirectory);

    ASSERT_FALSE(result.has_value());
    ASSERT_EQ(result.error(), "There is no configuration file found.");
}

TEST_F(ConfigUtilsTest, createFilePaths_withProductionEnv_sortsCorrectly)
{
    EnvironmentSetter::setEnvironmentVariable("CXX_ENV", "production");

    // Create files in random order
    std::ofstream{testConfigDirectory / "local.json"} << "{}";
    std::ofstream{testConfigDirectory / "production.json"} << "{}";
    std::ofstream{testConfigDirectory / "default.json"} << "{}";
    std::ofstream{testConfigDirectory / "custom-environment-variables.json"} << "{}";
    std::ofstream{testConfigDirectory / "local-production.json"} << "{}";

    const auto result = createFilePaths(testConfigDirectory);

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 5);
    ASSERT_EQ(result->at(0).filename(), "default.json");
    ASSERT_EQ(result->at(1).filename(), "production.json");
    ASSERT_EQ(result->at(2).filename(), "local.json");
    ASSERT_EQ(result->at(3).filename(), "local-production.json");
    ASSERT_EQ(result->at(4).filename(), "custom-environment-variables.json");
}

TEST_F(ConfigUtilsTest, createFilePaths_withMixedExtensions_includesAllRegularFiles)
{
    EnvironmentSetter::setEnvironmentVariable("CXX_ENV", "development");

    std::ofstream{testConfigDirectory / "default.json"} << "{}";
    std::ofstream{testConfigDirectory / "default.yaml"} << "{}";
    std::ofstream{testConfigDirectory / "default.xml"} << "{}";

    const auto result = createFilePaths(testConfigDirectory);

    ASSERT_TRUE(result.has_value());
    // All three files should be included and sorted by name (all have "default" stem)
    ASSERT_EQ(result->size(), 3);
}

// ==================== BACKWARD COMPATIBILITY TESTS ====================

TEST_F(ConfigUtilsTest, regularFile_lambda_checksRegularFile)
{
    std::ofstream{testConfigDirectory / "test.json"} << "{}";
    std::filesystem::create_directory(testConfigDirectory / "testdir");

    ASSERT_TRUE(regular_file(testConfigDirectory / "test.json"));
    ASSERT_FALSE(regular_file(testConfigDirectory / "testdir"));
}

TEST_F(ConfigUtilsTest, customFileOrder_lambda_sortsCorrectly)
{
    EnvironmentSetter::setEnvironmentVariable("CXX_ENV", "development");

    std::filesystem::path default_json = "default.json";
    std::filesystem::path development_json = "development.json";

    ASSERT_TRUE(customFileOrder(default_json, development_json));
    ASSERT_FALSE(customFileOrder(development_json, default_json));
}

