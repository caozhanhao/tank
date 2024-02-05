cmake_minimum_required(VERSION 3.8.2)
project(tank)
set(CMAKE_CXX_STANDARD 20)
find_package(Threads REQUIRED)
include_directories(include)
add_executable(tank
        src/main.cpp
        src/game.cpp
        src/game_map.cpp
        src/tank.cpp
        src/bullet.cpp
        src/command.cpp
        src/term.cpp
        src/renderer.cpp
        src/input.cpp
        src/online.cpp
        src/utils.cpp
        src/message.cpp
)
target_link_libraries(tank Threads::Threads)