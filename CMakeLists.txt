cmake_minimum_required(VERSION 3.25)
project(curaengine_plugin_gradual_flow)

find_package(Protobuf REQUIRED)
find_package(spdlog REQUIRED)
find_package(docopt REQUIRED)
find_package(asio-grpc REQUIRED)
find_package(range-v3 REQUIRED)
find_package(clipper REQUIRED)
find_package(ctre REQUIRED)
find_package(neargye-semver REQUIRED)
find_package(curaengine_grpc_definitions REQUIRED)

set(HDRS include/gradual_flow/boost_tags.h
        include/gradual_flow/concepts.h
        include/gradual_flow/gcode_path.h
        include/gradual_flow/point_container.h
        include/gradual_flow/utils.h
        include/plugin/broadcast.h
        include/plugin/cmdline.h
        include/plugin/handshake.h
        include/plugin/metadata.h
        include/plugin/modify.h
        include/plugin/plugin.h
        include/plugin/settings.h)

add_library(curaengine_plugin_gradual_flow_lib INTERFACE ${HDRS})

add_executable(curaengine_plugin_gradual_flow src/main.cpp)

target_include_directories(curaengine_plugin_gradual_flow_lib
        INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        )

set(DEPS curaengine_grpc_definitions::curaengine_grpc_definitions clipper::clipper ctre::ctre asio-grpc::asio-grpc protobuf::libprotobuf boost::boost spdlog::spdlog docopt_s range-v3::range-v3 neargye-semver::neargye-semver)

target_link_libraries(curaengine_plugin_gradual_flow PUBLIC curaengine_plugin_gradual_flow_lib ${DEPS})

option(ENABLE_TESTS "Build with unit test" ON)

if (ENABLE_TESTS)
        message(STATUS "curaengine_plugin_gradual_flow: Compiling with Tests")
        enable_testing()
        add_subdirectory(tests)
endif ()