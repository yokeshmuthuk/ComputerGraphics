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
# glm
# -----------------------------
include_directories("${CMAKE_CURRENT_LIST_DIR}/external/glm-0.9.7.1")

# -----------------------------
# Assimp
# -----------------------------
set(ASSIMP_SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/external/assimp")
if (EXISTS "${ASSIMP_SRC_DIR}/CMakeLists.txt")
    message(STATUS "Using vendored Assimp at: ${ASSIMP_SRC_DIR}")

    set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)

    # Only importers you need
    set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT  OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT  OFF CACHE BOOL "" FORCE)
    set(ASSIMP_NO_EXPORT                        ON  CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_COLLADA_IMPORTER           ON  CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_OBJ_IMPORTER               ON  CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_FBX_IMPORTER               OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_GLTF_IMPORTER              OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_GLTF_EXPORTER              OFF CACHE BOOL "" FORCE)

    add_subdirectory("${ASSIMP_SRC_DIR}" "${CMAKE_BINARY_DIR}/assimp-build")

    if (TARGET assimp AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(assimp PRIVATE -Wno-error=dangling-reference -Wno-dangling-reference)
    endif()
else()
    message(FATAL_ERROR "Assimp not found at ${ASSIMP_SRC_DIR}")
endif()

# -----------------------------
# Lab 4 executable only
# -----------------------------
add_executable(lab4_main
        "source code/main.cpp"
        "source code/maths_funcs.cpp"
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

# Copy assets/shaders next to the exe
add_custom_command(TARGET lab4_main POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_LIST_DIR}/source code/simpleVertexShader.txt"
        "${CMAKE_CURRENT_LIST_DIR}/source code/simpleFragmentShader.txt"
        "${CMAKE_CURRENT_LIST_DIR}/source code/monkeyhead_smooth.dae"
        $<TARGET_FILE_DIR:lab4_main>
)
