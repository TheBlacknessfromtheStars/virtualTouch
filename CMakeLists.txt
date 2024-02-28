cmake_minimum_required(VERSION 3.27)
project(AndroidTouch C)

set(CMAKE_C_STANDARD 11)

add_executable(AndroidTouch main.c
        touch.c)
