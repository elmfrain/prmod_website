cmake_minimum_required(VERSION 3.2)

project(pr_web)

#Emscripten enviornment and build flags
if(EMSCRIPTEN)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-implicit-function-declaration")
    set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -s USE_GLFW=3 --preload-file res")
    set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -s ALLOW_MEMORY_GROWTH=1")
    set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -s USE_WEBGL2=1 -s FETCH=1")
    set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -s EXPORTED_FUNCTIONS=_main,_prwuiResize -s EXPORTED_RUNTIME_METHODS=ccall")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

    #Copy additional files needed to display program in a browser
    file(COPY emscripten/index.html DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
    file(COPY emscripten/htconfig.json DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
    file(COPY emscripten/favicon.ico DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
endif()

#Add json-c
set(BUILD_SHARED_LIBS         OFF CACHE BOOL " " FORCE)
set(DISABLE_EXTRA_LIBS        ON  CACHE BOOL " " FORCE)
set(ENABLE_THREADING          OFF CACHE BOOL " " FORCE)
add_subdirectory("dep/json-c")

#Add glm
option(CGLM_SHARED "Shared build" OFF)
option(CGLM_STATIC "Static build" ON)
add_subdirectory("dep/cglm")

#Add assimp
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL " " FORCE)

#Only enable obj and ply importer
set(ASSIMP_BUILD_OBJ_IMPORTER ON  CACHE BOOL " " FORCE)
set(ASSIMP_BUILD_PLY_IMPORTER ON  CACHE BOOL " " FORCE)

set(ASSIMP_NO_EXPORT          ON  CACHE BOOL " " FORCE)
set(ASSIMP_BUILD_TESTS        OFF CACHE BOOL " " FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL " " FORCE)
set(ASSIMP_INSTALL            OFF CACHE BOOL " " FORCE)
set(BUILD_SHARED_LIBS         OFF CACHE BOOL " " FORCE)
add_subdirectory("dep/assimp")

if(NOT EMSCRIPTEN)
    #Add glad for OpenGL
    add_subdirectory("dep/glad")

    #add glfw
    add_subdirectory("dep/glfw")
endif()

if(NOT EMSCRIPTEN)
    #add ssl
    set(OPENSSL_USE_STATIC_LIBS TRUE)
    find_package(OpenSSL REQUIRED)
endif()

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
    downloads_view.c
    mesh_builder.c
)

if(UNIX AND NOT EMSCRIPTEN)
    target_link_libraries(${PROJECT_NAME}
        glad
        cglm
        glfw
        ${OPENSSL_SSL_LIBRARY}
        ${OPENSSL_LIBRARIES}
        pthread
        assimp
        json-c
        m
    )
elseif(WIN32)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_link_libraries(${PROJECT_NAME}
            glad
            cglm
            glfw
            ${OPENSSL_SSL_LIBRARY}
            ${OPENSSL_LIBRARIES}
            pthread
            assimp
            json-c
            m
            ws2_32
        )
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_link_libraries(${PROJECT_NAME}
            glad
            cglm
            glfw
            ${OPENSSL_SSL_LIBRARY}
            ${OPENSSL_LIBRARIES}
            assimp
            json-c
            ws2_32
        )
    endif()
else()
	target_link_libraries(${PROJECT_NAME}
        cglm
        glfw
        assimp
        json-c
        m
    )
endif()

if(NOT EMSCRIPTEN)
    target_include_directories(${PROJECT_NAME} PRIVATE
        include
        dep/glfw/include
        dep/glad/include
        dep/cglm/include
        ${OPENSSL_INCLUDE_DIR}
        dep/json-c
    )
else()
    target_include_directories(${PROJECT_NAME} PRIVATE
        include
        dep/cglm/include
        dep/openssl/include
        dep/json-c
    )
endif()

#Copy resource files to current binary folder
if(NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)

    file(GLOB_RECURSE resfiles RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "res/*")

    foreach(resfile ${resfiles})
    
        add_custom_command(
            TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_CURRENT_SOURCE_DIR}/${resfile}"
            "${CMAKE_CURRENT_BINARY_DIR}/${resfile}"
        )

    endforeach(resfile)

    file(COPY res DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
    
endif()
