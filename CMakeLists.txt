cmake_minimum_required(VERSION 3.2)

project(pr_web)

#Add glad for OpenGL
add_subdirectory("dep/glad")

#Add glm
add_subdirectory("dep/glm")

#add glfw
add_subdirectory("dep/glfw")

add_executable(${PROJECT_NAME}
    main.c
)

target_link_libraries(${PROJECT_NAME}
    glad
    glm
    glfw
)

target_include_directories(${PROJECT_NAME} PRIVATE
    include
    dep/glfw/include
    dep/glad/include
    dep/glm/glm
)