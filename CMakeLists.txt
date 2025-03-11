cmake_minimum_required(VERSION 3.30)
project(OpenGLProject)
include_directories(include)
set(CMAKE_CXX_STANDARD 20)

add_executable(OpenGLProject main.cpp src/glad.c
        shader.h
        stb_image.h
        camera.h
        block.cpp
        PerlinNoise.hpp
        chunk.cpp
        settings.cpp
        chunk.h
        imconfig.h
        imgui.cpp
        imgui.h
        imgui_demo.cpp
        imgui_draw.cpp
        imgui_impl_glfw.h
        imgui_impl_opengl3.h
        imgui_internal.h
        imgui_tables.cpp
        imgui_widgets.cpp
        imstb_rectpack.h
        imstb_textedit.h
        imstb_truetype.h
        imgui_impl_opengl3.cpp
        imgui_impl_opengl3_loader.h
        imgui_impl_glfw.cpp
        FastNoiseLite.h
        InGameHUD.cpp
        InGameHUD.h)
target_link_libraries(OpenGLProject GL GLEW glfw)
