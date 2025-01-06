cmake_minimum_required(VERSION 3.30)
project(OpenGLProject)
include_directories(include)
set(CMAKE_CXX_STANDARD 20)

add_executable(OpenGLProject main.cpp src/glad.c
        shader.h
        stb_image.h
        camera.h
        block.cpp
        PerlinNoise.hpp)
target_link_libraries(OpenGLProject GL GLEW glfw)
