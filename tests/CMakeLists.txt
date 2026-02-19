find_package(GTest REQUIRED)

enable_testing()
add_executable(tests
    config_test.cpp
    config_directory_path_resolver_test.cpp
    json_config_loader_test.cpp
    yaml_config_loader_test.cpp
    xml_config_loader_test.cpp
    config_provider_test.cpp
    file_system_service_test.cpp
    file_system_service_executable_test.cpp
    environment_setter.cpp
    environment_setter_test.cpp
)

target_link_libraries(tests
    PUBLIC
    GTest::gtest_main
    config-cxx
    PRIVATE
    nlohmann_json::nlohmann_json
)

target_include_directories(tests PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)

include(GoogleTest)
gtest_discover_tests(tests)

if (CONFIG_CODE_COVERAGE)
    target_code_coverage(${CMAKE_PROJECT_NAME}-UT ALL)
endif ()
