cmake_minimum_required(VERSION 3.20)
project(lab4)

set(CMAKE_CXX_STANDARD 11)
find_package(OpenGL REQUIRED)

# Output dirs
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

# -----------------------------
# GLFW
# -----------------------------
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/external/glfw-3.4")

# -----------------------------
# GLAD
# -----------------------------
set(GLAD_ROOT "${CMAKE_CURRENT_LIST_DIR}/external/glad-3.3")
set(GLAD_INC  "${GLAD_ROOT}/include")
if (EXISTS "${GLAD_ROOT}/src/glad.c" AND EXISTS "${GLAD_INC}/glad/glad.h")
    message(STATUS "GLAD: using GLAD1 (glad/glad.h + src/glad.c)")
    add_library(glad "${GLAD_ROOT}/src/glad.c")
    target_include_directories(glad PUBLIC "${GLAD_INC}")
elseif (EXISTS "${GLAD_ROOT}/src/gl.c" AND EXISTS "${GLAD_INC}/glad/gl.h")
    message(STATUS "GLAD: using GLAD2 (glad/gl.h + src/gl.c)")
    add_library(glad "${GLAD_ROOT}/src/gl.c")
    target_include_directories(glad PUBLIC "${GLAD_INC}")
else()
    message(FATAL_ERROR "GLAD sources not found in ${GLAD_ROOT}")
endif()

# -----------------------------
# Assimp (for orbital ring model loading)
# -----------------------------
find_package(assimp REQUIRED)
if(assimp_FOUND)
    message(STATUS "Found Assimp: ${assimp_VERSION}")
else()
    message(FATAL_ERROR "Assimp not found. Please install Assimp library.")
endif()

# -----------------------------
# glm
# -----------------------------
include_directories("${CMAKE_CURRENT_LIST_DIR}/external/glm-0.9.7.1")

# -----------------------------
# Lab 4 executable only
# -----------------------------
add_executable(lab4_main
        "source code/main.cpp"
        "source code/maths_funcs.cpp"
        "source code/sphere_generator.cpp"
)

target_include_directories(lab4_main PRIVATE
        "${GLAD_INC}"
        "${CMAKE_CURRENT_LIST_DIR}/external/glfw-3.4/include"
        "${CMAKE_CURRENT_LIST_DIR}/external/glm-0.9.7.1"
        "${CMAKE_CURRENT_LIST_DIR}/source code"
)

target_link_libraries(lab4_main PRIVATE
        ${OPENGL_LIBRARY}
        glfw
        glad
        assimp::assimp
)

# Set EARTH_TEXTURE variable
set(EARTH_TEXTURE "earth_color.jpg")

# Copy assets/shaders next to the exe
add_custom_command(TARGET lab4_main POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_LIST_DIR}/source code/simpleVertexShader.txt"
        "${CMAKE_CURRENT_LIST_DIR}/source code/simpleFragmentShader.txt"
        "${CMAKE_CURRENT_LIST_DIR}/source code/skyboxVertexShader.txt"
        "${CMAKE_CURRENT_LIST_DIR}/source code/skyboxFragmentShader.txt"
        "${CMAKE_CURRENT_LIST_DIR}/source code/skyboxEquirectFragmentShader.txt"
        $<TARGET_FILE_DIR:lab4_main>
        COMMENT "Copying shader files to build directory"
)

# Copy Earth texture if it exists
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/source code/${EARTH_TEXTURE}")
    add_custom_command(TARGET lab4_main POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_LIST_DIR}/source code/${EARTH_TEXTURE}"
            $<TARGET_FILE_DIR:lab4_main>
            COMMENT "Copying Earth texture to build directory"
    )
else()
    message(STATUS "Note: ${EARTH_TEXTURE} not found - app will use procedural fallback texture")
endif()

# Copy skybox folder if it exists
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/source code/skybox")
    add_custom_command(TARGET lab4_main POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_LIST_DIR}/source code/skybox"
            "$<TARGET_FILE_DIR:lab4_main>/skybox"
            COMMENT "Copying skybox images to build directory"
    )
    message(STATUS "Skybox folder found - will be copied to build directory")
else()
    message(STATUS "Note: skybox folder not found - app will use procedural fallback skybox")
endif()

# Copy orbital ring model folder if it exists
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/source code/orbital_ring")
    add_custom_command(TARGET lab4_main POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_LIST_DIR}/source code/orbital_ring"
            "$<TARGET_FILE_DIR:lab4_main>/orbital_ring"
            COMMENT "Copying orbital ring model and textures to build directory"
    )
    message(STATUS "Orbital ring folder found - will be copied to build directory")
else()
    message(STATUS "Note: orbital ring folder not found - ring will not be rendered")
endif()
