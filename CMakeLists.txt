cmake_minimum_required(VERSION 3.16)
project(mandelbrot CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)
find_package(SFML 2.5 REQUIRED COMPONENTS graphics window)

if(CMAKE_CXX_COMPILER_ID MATCHES "(GNU|Clang)")
    add_compile_options(-ffast-math -Werror -Wall -Wextra -Wpedantic -Wshadow -Wsign-conversion)
endif()

add_executable(mandelbrot src/mandelbrot.cpp)
target_link_libraries(mandelbrot PRIVATE sfml-graphics sfml-window Threads::Threads)
target_compile_definitions(mandelbrot PRIVATE FONT_PATH="${CMAKE_SOURCE_DIR}/data")
target_compile_features(mandelbrot PRIVATE cxx_std_20)

add_custom_target(format
    COMMAND clang-format -i `git ls-files *.cpp *.h`
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
add_custom_target(tidy
    COMMAND clang-tidy -p ${CMAKE_BINARY_DIR} `git ls-files *.cpp`
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
