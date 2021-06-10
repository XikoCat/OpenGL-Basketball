cmake_minimum_required(VERSION 3.15)

#set the project name
project(CGr)

#OpenGL Flags
set(OpenGL_GL_PREFERENCE_GLVND)

find_package(OpenGL REQUIRED)

#specify C++ standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

#Compile external dependecies
add_subdirectory(external)

include_directories(
    external/glfw-3.3.3/include/GLFW/
    external/glad/include/glad/
    external/glm/
)

set(ALL_LIBS
    ${OPENGL_LIBRARY}
    glfw
    glad
    glm
)

add_subdirectory(proj)