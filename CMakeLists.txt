cmake_minimum_required(VERSION 4.0)
project(config)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCES "src/lib/*.h" "src/lib/*.cpp" "src/lib/*.hpp")

add_library(config-lib INTERFACE ${SOURCES})
target_include_directories(config-lib INTERFACE src/lib)

add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE config-lib)
target_include_directories(${PROJECT_NAME} PRIVATE src)
