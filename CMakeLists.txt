cmake_minimum_required(VERSION 3.22)
project(mandelbrot CXX)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

if(CMAKE_CXX_COMPILER_ID MATCHES "(GNU|Clang)")
    add_compile_options(-ffast-math -Werror -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Wold-style-cast)
elseif(MSVC)
    add_compile_options(/WX /W4 /permissive-)
endif()

include(FetchContent)
set(SFML_BUILD_NETWORK OFF)
FetchContent_Declare(SFML GIT_REPOSITORY https://github.com/SFML/SFML.git GIT_TAG ae87def93e7b0280b5826b0fa5e1a8ef7156e9e7)
FetchContent_MakeAvailable(SFML)

add_executable(mandelbrot src/Mandelbrot.cpp)
target_link_libraries(mandelbrot PRIVATE SFML::Graphics SFML::Audio Threads::Threads)

add_custom_target(format
    COMMAND clang-format -i `git ls-files *.hpp *.cpp`
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
add_custom_target(tidy
    COMMAND run-clang-tidy -p ${CMAKE_BINARY_DIR} `git ls-files *.cpp`
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
add_custom_target(run
    COMMAND mandelbrot
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
