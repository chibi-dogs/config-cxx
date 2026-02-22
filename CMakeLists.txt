cmake_minimum_required(VERSION 3.22)

set(PROJECT_NAME config-cxx)
project(${PROJECT_NAME} CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(CONFIG_BUILD_TESTING "Build tests" ON)
option(CONFIG_CODE_COVERAGE "Build config-cxx with coverage support" OFF)

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++20 /permissive- /bigobj")
else ()
    set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Wconversion -Wformat"
    )
endif ()

set(LIBRARY_NAME config-cxx)

set(SOURCES
    src/config.cpp
    src/config_directory_path_resolver.cpp
    src/config_provider.cpp
    src/file_system_service.cpp
    src/json_config_loader.cpp
    src/yaml_config_loader.cpp
    src/xml_config_loader.cpp
)

add_library(${LIBRARY_NAME} ${SOURCES})

find_package(yaml-cpp REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(pugixml REQUIRED)

target_link_libraries(${LIBRARY_NAME} PUBLIC
        yaml-cpp::yaml-cpp
        pugixml::pugixml
        PRIVATE
        nlohmann_json::nlohmann_json
)

target_include_directories(
    ${LIBRARY_NAME}
    PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include"
    PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src"
)

if (CONFIG_CODE_COVERAGE)
    set(target_code_coverage_ALL 1)
    include("cmake/cmake-coverage.cmake")
    add_code_coverage_all_targets(EXCLUDE tests/*)
    target_code_coverage(${LIBRARY_NAME} ALL)
endif ()

if (CONFIG_BUILD_TESTING)
    add_subdirectory(tests)
endif ()

add_subdirectory(examples/project)