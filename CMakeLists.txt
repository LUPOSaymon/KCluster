cmake_minimum_required(VERSION 3.20)
project(KCluster C)

set(CMAKE_C_STANDARD 99)

add_executable(KCluster main.c)
INCLUDE_DIRECTORIES(
        /usr/allegro/include
)

LINK_DIRECTORIES(
        /usr/allegro/lib
)

TARGET_LINK_LIBRARIES(KCluster
        liballegro.a
        )
