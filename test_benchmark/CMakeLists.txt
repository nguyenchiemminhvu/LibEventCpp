cmake_minimum_required(VERSION 3.11...3.20)
project(LibEventCppBenchmark)

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF)
set(BENCHMARK_ENABLE_TESTING OFF)

find_package(Threads)

include(FetchContent)

FetchContent_Declare(
    google_benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG main
)

FetchContent_Declare(
    libevent_cpp
    GIT_REPOSITORY https://github.com/nguyenchiemminhvu/LibEventCpp.git
    GIT_TAG main
)

FetchContent_MakeAvailable(google_benchmark libevent_cpp)

function(compile_executables target_name file_path)
    add_executable(${target_name} ${file_path})
    target_link_libraries(${target_name}
        PRIVATE
            benchmark::benchmark
            Threads::Threads
    )
    target_include_directories(${target_name} PRIVATE ${libevent_cpp_SOURCE_DIR})
endfunction()

compile_executables(benchmark_event_handler ${CMAKE_CURRENT_SOURCE_DIR}/benchmark_event_handler.cpp)
compile_executables(benchmark_sigslot ${CMAKE_CURRENT_SOURCE_DIR}/benchmark_sigslot.cpp)
