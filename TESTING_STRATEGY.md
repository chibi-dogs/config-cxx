# Testing Strategy for config_utils

## Overview

This document explains the improved testing approach for functions in `config_utils.h` that rely on filesystem operations. The original code had several testability issues that have been addressed in the refactored version.

## Problems with Original Implementation

### 1. **Tight Coupling to Filesystem**
The `createFilePaths` function directly used:
- `std::filesystem::directory_iterator`
- `filesystem_utils::is_directory_empty`
- `is_regular_file`

This made it difficult to test without creating actual files and directories.

### 2. **Hidden Dependencies**
The function had implicit dependencies:
- `ConfigProvider::getCxxEnv()` - reads environment variables
- `std::getenv("SUPPRESS_NO_CONFIG_WARNING")` - reads environment
- Filesystem state

### 3. **Mixed Concerns**
Business logic (sorting order, filtering) was mixed with I/O operations, making unit testing challenging.

### 4. **Lambda Functions with Side Effects**
`customFileOrder` lambda called `ConfigProvider::getCxxEnv()` on every comparison, making it:
- Hard to test in isolation
- Inefficient (multiple env reads during sorting)
- Difficult to mock

## Refactored Solution

### Key Improvements

#### 1. **Dependency Injection**
```cpp
std::expected<std::vector<std::filesystem::path>, std::string> createFilePathsWithDeps(
    const std::filesystem::path& configFolder,
    const std::string& cxxEnv,              // Injected instead of calling ConfigProvider
    EmptyDirChecker isEmptyChecker,         // Mockable filesystem check
    DirectoryReader dirReader,               // Mockable directory reading
    FileFilter fileFilter,                   // Mockable file filtering
    bool suppressWarning = false)           // Explicit parameter instead of getenv
```

Benefits:
- Easy to mock dependencies in tests
- No filesystem needed for unit tests
- No environment variable setup required
- Pure business logic testing

#### 2. **Separated Pure Functions**

**`getFileOrder(cxxEnv)`** - Pure function
- Input: environment string
- Output: file order vector
- No side effects, fully deterministic
- Easy to test

**`compareByCustomOrder(path1, path2, order)`** - Pure function  
- Input: two paths and an order vector
- Output: boolean comparison result
- No side effects
- Testable with any paths (even fake ones)

#### 3. **Type Aliases for Clarity**
```cpp
using FileFilter = std::function<bool(const std::filesystem::path&)>;
using FileComparator = std::function<bool(const std::filesystem::path&, const std::filesystem::path&)>;
using DirectoryReader = std::function<std::vector<std::filesystem::path>(const std::filesystem::path&)>;
using EmptyDirChecker = std::function<bool(const std::filesystem::path&)>;
```

Makes function signatures clearer and dependencies explicit.

#### 4. **Backward Compatibility**
The original `createFilePaths` function is preserved:
```cpp
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
```

This delegates to the testable version with real dependencies.

## Testing Strategy

### Level 1: Pure Function Tests (No Dependencies)

These test business logic without any filesystem or environment:

```cpp
TEST_F(ConfigUtilsTest, getFileOrder_withDevelopmentEnv_returnsCorrectOrder)
{
    const auto order = getFileOrder("development");
    std::vector<std::string> expected = {"default", "development", "local", 
                                         "local-development", "custom-environment-variables"};
    ASSERT_EQ(order, expected);
}

TEST_F(ConfigUtilsTest, compareByCustomOrder_bothInOrder_sortsCorrectly)
{
    const std::vector<std::string> order = {"default", "development", "local"};
    std::filesystem::path default_json = "default.json";
    std::filesystem::path development_json = "development.json";
    
    ASSERT_TRUE(compareByCustomOrder(default_json, development_json, order));
}
```

**Benefits:**
- Extremely fast (no I/O)
- No setup/teardown
- No flaky tests
- Easy to test edge cases

### Level 2: Dependency Injection Tests (Mocked Dependencies)

These test the core algorithm with mocked filesystem behavior:

```cpp
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
        "/fake/path", "development",
        mockIsEmpty, mockReader, mockFilter, false);
    
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->at(0).filename(), "default.json");
    ASSERT_EQ(result->at(1).filename(), "development.json");
    ASSERT_EQ(result->at(2).filename(), "local.json");
    ASSERT_EQ(result->at(3).filename(), "custom-environment-variables.json");
}
```

**Benefits:**
- Tests the full algorithm without filesystem
- Easy to test error conditions (empty directory, no files, etc.)
- Fast execution
- Deterministic
- Easy to test edge cases (many files, unusual names, etc.)

### Level 3: Integration Tests (Real Filesystem)

These verify the system works with actual filesystem:

```cpp
TEST_F(ConfigUtilsTest, createFilePaths_withRealFilesystem_worksCorrectly)
{
    EnvironmentSetter::setEnvironmentVariable("CXX_ENV", "development");
    
    std::ofstream{testConfigDirectory / "default.json"} << "{}";
    std::ofstream{testConfigDirectory / "development.json"} << "{}";
    std::ofstream{testConfigDirectory / "local.json"} << "{}";
    
    const auto result = createFilePaths(testConfigDirectory);
    
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 3);
}
```

**Benefits:**
- Confirms real-world behavior
- Tests integration with actual filesystem APIs
- Catches platform-specific issues

## Test Coverage Matrix

| Test Type | Speed | Setup Complexity | Reliability | Coverage |
|-----------|-------|------------------|-------------|----------|
| Pure Functions | ⚡⚡⚡ | None | 100% | Business logic |
| Mocked Dependencies | ⚡⚡ | Low | 99% | Full algorithm |
| Integration | ⚡ | High | 95% | Real behavior |

## Best Practices Applied

### 1. **Test Pyramid**
- Many pure function tests (fast, reliable)
- Moderate mocked dependency tests (good coverage)
- Few integration tests (verify real behavior)

### 2. **Arrange-Act-Assert Pattern**
All tests follow clear structure:
```cpp
// Arrange - setup mocks/data
auto mockIsEmpty = [](const std::filesystem::path&) { return false; };

// Act - execute the function
const auto result = createFilePathsWithDeps(...);

// Assert - verify results
ASSERT_TRUE(result.has_value());
```

### 3. **Descriptive Test Names**
- `createFilePathsWithDeps_multipleFiles_sortedCorrectly`
- `compareByCustomOrder_bothNotInOrder_sortsAlphabetically`

Names clearly describe: function, scenario, expected outcome

### 4. **Edge Case Coverage**
- Empty directories
- No regular files (only directories)
- Unknown file names
- Mixed known and unknown files
- Single file
- Different environments

### 5. **Isolation**
Each test:
- Sets up its own data
- Cleans up after itself
- Doesn't depend on other tests
- Can run in any order

## Migration Guide

To apply this pattern to other parts of the codebase:

### Step 1: Identify Dependencies
Look for:
- Filesystem operations
- Environment variable reads
- Network calls
- Database queries
- Time/date functions

### Step 2: Extract Pure Functions
Separate business logic from I/O:
```cpp
// Before
auto getConfig() {
    auto env = getenv("ENV");
    auto files = readDirectory(path);
    return process(files, env);
}

// After
auto processFiles(vector<path> files, string env) { ... }  // Pure
auto getConfig() {
    return processFiles(readDirectory(path), getenv("ENV"));  // Thin wrapper
}
```

### Step 3: Add Dependency Injection
```cpp
// Testable version with injected dependencies
auto processFilesWithDeps(
    vector<path> files,
    string env,
    FileReader reader,
    EnvProvider envProvider) { ... }

// Production version
auto processFiles(vector<path> files, string env) {
    return processFilesWithDeps(files, env, defaultReader, defaultEnvProvider);
}
```

### Step 4: Write Tests at All Levels
1. Pure function tests (no dependencies)
2. Mocked dependency tests (injected mocks)
3. Integration tests (real dependencies)

## Running the Tests

```bash
# Build tests
cmake --build cmake-build-debug --target config-cxx-UT

# Run all config_utils tests
./cmake-build-debug/tests/config-cxx-UT --gtest_filter="ConfigUtilsTest.*"

# Run only pure function tests (fastest)
./cmake-build-debug/tests/config-cxx-UT --gtest_filter="ConfigUtilsTest.getFileOrder*:ConfigUtilsTest.compareByCustomOrder*"

# Run only mocked tests
./cmake-build-debug/tests/config-cxx-UT --gtest_filter="ConfigUtilsTest.createFilePathsWithDeps*"

# Run only integration tests
./cmake-build-debug/tests/config-cxx-UT --gtest_filter="ConfigUtilsTest.createFilePaths_*"
```

## Benefits Summary

### For Development
- Faster test execution (pure function tests are instant)
- Easier debugging (can test logic without filesystem setup)
- Better code organization (separation of concerns)

### For Maintenance  
- Easier to understand (clear dependencies)
- Easier to modify (change logic without breaking tests)
- Easier to extend (add new file types, sorting rules, etc.)

### For Quality
- Higher test coverage (can test edge cases easily)
- More reliable tests (fewer flaky filesystem tests)
- Better error handling (can test error paths easily)

## Conclusion

By separating pure business logic from I/O operations and using dependency injection, we've made the code:
- **More testable**: Can test without filesystem
- **More maintainable**: Clear dependencies and concerns
- **More reliable**: Fewer flaky tests
- **Faster to test**: Pure functions test in microseconds

This pattern can be applied throughout the codebase to improve testability while maintaining backward compatibility.

