cmake_minimum_required(VERSION 3.18 FATAL_ERROR)

project(DivideAndConquer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if (MSVC)
    # warning level 4 and all warnings as errors
    add_compile_options(/W4 /WX)
endif()

find_package(cppzmq 4.7.1 REQUIRED)

add_executable(Ventilator taskvent.cpp)
target_link_libraries(Ventilator cppzmq)

add_executable(Worker taskwork.cpp)
target_link_libraries(Worker cppzmq)

add_executable(Sink tasksink.cpp)
target_link_libraries(Sink cppzmq)
