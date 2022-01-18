cmake_minimum_required(VERSION 3.20)

project(KClusterSerial C)
set(CMAKE_C_STANDARD 99)
add_executable(KClusterSerial mainSerial.c)
target_link_directories(KClusterSerial PUBLIC "/usr/include/allegro5")
target_link_libraries(KClusterSerial PUBLIC allegro allegro_main allegro_dialog allegro_primitives m )

project(KClusterParallel C)
add_executable(KClusterParallel mainParallel.c)
target_link_directories(KClusterParallel PUBLIC "/usr/include/allegro5")
target_link_libraries(KClusterParallel PUBLIC allegro allegro_main allegro_dialog allegro_primitives m pthread)