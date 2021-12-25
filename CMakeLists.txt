cmake_minimum_required(VERSION 3.20)
project(KCluster C)

set(CMAKE_C_STANDARD 99)

add_executable(KCluster main.c)
target_link_directories(KCluster PUBLIC "/usr/include/allegro5")
target_link_libraries(KCluster PUBLIC allegro allegro_main allegro_dialog allegro_primitives m )
