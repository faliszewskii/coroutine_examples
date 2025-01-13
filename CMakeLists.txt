cmake_minimum_required(VERSION 3.29)
project(coroutines)

set(CMAKE_CXX_STANDARD 26)

add_executable(coroutines main.cpp
        async_work.h
        fibonacci.h
        iterator.h
        state_machine.h)
