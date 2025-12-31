cmake_minimum_required(VERSION 3.22)
project(config)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(config INTERFACE)
add_library(config::config ALIAS config)
target_include_directories(config
        INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/include>
        $<INSTALL_INTERFACE:include>)
target_compile_features(config INTERFACE cxx_std_23)

option(CONFIG_BUILD_EXAMPLES "Build config examples" OFF)

if(CONFIG_BUILD_EXAMPLES)
    add_executable(config-example
            src/main.cpp
    )
    target_link_libraries(config-example PRIVATE config::config)
    target_include_directories(config-example PRIVATE src)
endif()

### install
install(TARGETS config
        EXPORT configTargets)

install(DIRECTORY src/include/
        DESTINATION include)

install(EXPORT configTargets
        NAMESPACE config::
        DESTINATION lib/cmake/config)
