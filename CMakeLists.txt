cmake_minimum_required(VERSION 3.2)

project(pr_web)

#Add glad for OpenGL
add_subdirectory("dep/glad")

#Add glm
option(CGLM_SHARED "Shared build" OFF)
option(CGLM_STATIC "Static build" ON)
add_subdirectory("dep/cglm")

#add glfw
add_subdirectory("dep/glfw")

#add ssl
set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL REQUIRED)

add_executable(${PROJECT_NAME}
    main.c
    stb_image.c
    input.c
    shaders.c
    ui_render.c
    widget.c
    screen_renderer.c
    animation.c
    mesh.c
    https_fetcher.c
    title_view.c
    about_view.c
    markdown_viewer.c
    download_view.c
)

target_link_libraries(${PROJECT_NAME}
    glad
    cglm
    glfw
    ${OPENSSL_SSL_LIBRARY}
    ${OPENSSL_LIBRARIES}
    pthread
)

target_include_directories(${PROJECT_NAME} PRIVATE
    include
    dep/glfw/include
    dep/glad/include
    dep/cglm/include
)