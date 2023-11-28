cmake_minimum_required(VERSION 3.25.0)
project(anyc VERSION 0.1.0 LANGUAGES C)

include(CTest)
enable_testing()

set(CMAKE_C_STANDARD 17)

include(FetchContent)

FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.3.8
)

FetchContent_MakeAvailable(glfw)

find_package(Vulkan REQUIRED)

file(GLOB SOURCES "src/*.c" "src/**/*.c")
add_executable(anyc ${SOURCES})
target_link_libraries(anyc Vulkan::Vulkan glfw)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
