cmake_minimum_required(VERSION 3.18 FATAL_ERROR)

project(HelloWorld)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if (MSVC)
    # warning level 4 and all warnings as errors
    add_compile_options(/W4 /WX)
endif()

find_package(cppzmq 4.7.1 REQUIRED)

add_executable(Server hwserver.cpp)
target_link_libraries(Server cppzmq)

add_executable(Client hwclient.cpp)
target_link_libraries(Client cppzmq)
