cmake_minimum_required(VERSION 3.25.0)
project(anyc VERSION 0.1.0 LANGUAGES C)

include(CTest)
enable_testing()

set(CMAKE_C_STANDARD 17)

file(GLOB SOURCES "src/*.c" "src/**/*.c")

add_executable(anyc ${SOURCES})

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

target_link_libraries(anyc Vulkan::Vulkan glfw)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
