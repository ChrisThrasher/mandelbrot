cmake_minimum_required(VERSION 3.28)
project(mandelbrot CXX)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

include(FetchContent)
set(SFML_BUILD_NETWORK OFF)
FetchContent_Declare(SFML GIT_REPOSITORY https://github.com/SFML/SFML.git SYSTEM EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(SFML)

if(CMAKE_CXX_COMPILER_ID MATCHES "(GNU|Clang)")
    add_compile_options(-ffast-math -Werror -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Wold-style-cast)
elseif(MSVC)
    add_compile_options(/WX /W4 /permissive-)
endif()

add_executable(mandelbrot src/Mandelbrot.cpp)
target_link_libraries(mandelbrot PRIVATE SFML::Graphics SFML::Audio Threads::Threads)
target_compile_features(mandelbrot PRIVATE cxx_std_20)

add_custom_target(format
    COMMAND clang-format -i `git ls-files *.hpp *.cpp`
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
add_custom_target(tidy
    COMMAND run-clang-tidy -p ${CMAKE_BINARY_DIR} `git ls-files *.cpp`
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
add_custom_target(run
    COMMAND mandelbrot
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
