cmake_minimum_required(VERSION 3.22.1)

project(Ray)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -g -O2)
endif()

add_executable(${PROJECT_NAME} code/Ray.cpp)