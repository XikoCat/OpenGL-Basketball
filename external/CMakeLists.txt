# Compile external libraries
add_definitions(
        -DTW_STATIC
        -DTW_NO_LIB_PRAGMA
        -DTW_NO_DIRECT3D
        -DGLEW_STATIC
        -D_CRT_SECURE_NO_WARNINGS
)
### GLFW ###
add_subdirectory(glfw-3.3.3)
include_directories(
        glw-3.3.3/include/GLFW/
)
### GLAD ###
add_subdirectory(glad)
include_directories(
        glad/include/glad/
)

### GLM ###
add_subdirectory(glm-0.9.9.8)
include_directories(
	glm-0.9.9.8/glm/
)