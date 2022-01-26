cmake_minimum_required(VERSION 3.20)

project(KClusterParallel C)
add_executable(KClusterParallel mainParallel.c)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fopenmp")
target_link_directories(KClusterParallel PUBLIC "/usr/include/allegro5")
target_link_libraries(KClusterParallel PUBLIC allegro allegro_main allegro_dialog allegro_primitives m pthread)