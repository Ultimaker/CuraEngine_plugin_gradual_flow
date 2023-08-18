message(STATUS "Building tests...")
find_package(Catch2 REQUIRED)

include(CTest)
include(Catch)

set(SRC_TEST main.cpp)

add_executable(tests ${SRC_TEST})
target_link_libraries(tests PUBLIC ${DEPS} Catch2::Catch2WithMain curaengine_plugin_gradual_flow_lib)

catch_discover_tests(tests
        TEST_PREFIX
        "unittests."
        OUTPUT_DIR
        .
        OUTPUT_PREFIX
        "unittests."
        OUTPUT_SUFFIX
        .xml)