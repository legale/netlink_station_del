cmake_minimum_required(VERSION 3.24)
project(del_sta C)

set(CMAKE_C_STANDARD 11)

add_executable(del_sta
        station_del.c
        main.c)

include_directories(
        /usr/include
        /usr/include/libnl-tiny
)

target_link_libraries(del_sta
        nl-3
        nl-genl-3
        )