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
if (WIN32)
    target_link_libraries(tank wsock32 ws2_32 Threads::Threads)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  /DWIN32_LEAN_AND_MEAN")
    endif ()
else ()
    target_link_libraries(tank Threads::Threads)
endif ()