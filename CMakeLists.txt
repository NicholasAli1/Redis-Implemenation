cmake_minimum_required(VERSION 4.1)
project(redis)

set(CMAKE_CXX_STANDARD 20)

add_executable(redis src/main.cpp
        src/RedisServer.cpp
        include/RedisServer.h
        src/RedisCommandHandler.cpp
        include/RedisCommandHandler.h
        src/RedisDatabase.cpp
        include/RedisDatabase.h
)
