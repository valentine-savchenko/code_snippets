cmake_minimum_required(VERSION 3.18 FATAL_ERROR)

project(Reactor)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if (MSVC)
    # warning level 4 and all warnings as errors
    add_compile_options(/W4 /WX)
endif()

find_package(Boost 1.77.0 EXACT REQUIRED)

add_executable(Reactor
    main.cpp)

target_include_directories(Reactor PRIVATE
    ${Boost_INCLUDE_DIRS})

target_link_libraries(Reactor PRIVATE
    ${Boost_LIBRARIES})