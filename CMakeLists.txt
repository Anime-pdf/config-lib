cmake_minimum_required(VERSION 4.0)
project(config)

set(CMAKE_CXX_STANDARD 23)

file(GLOB_RECURSE SOURCES "src/lib/*.h" "src/lib/*.cpp")

add_library(config-lib STATIC ${SOURCES})
set_target_properties(config-lib PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(config-lib PRIVATE src)

add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PUBLIC config-lib)

set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(${PROJECT_NAME} PRIVATE src)
