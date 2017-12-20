cmake_minimum_required(VERSION 2.6)
project(sdlplayyuv)

add_executable(sdlplayyuv main.cpp)

install(TARGETS sdlplayyuv RUNTIME DESTINATION bin)
