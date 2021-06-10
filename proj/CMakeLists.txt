file(COPY models DESTINATION .)
file(COPY shaders DESTINATION .)
file(COPY textures DESTINATION .)
file(COPY animations DESTINATION .)

add_executable(basket.exe main.cpp)

target_link_libraries(basket.exe
    LINK_PUBLIC
        glfw
        glad
        glm
        ${CMAKE_DL_LIBS}
)
