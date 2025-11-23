#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <iostream>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "maths_funcs.h"


#include "maths_funcs.h"

#define MESH_NAME "monkeyhead_smooth.dae"

struct ModelData {
    size_t mPointCount = 0;
    std::vector<vec3> mVertices;
    std::vector<vec3> mNormals;
    std::vector<vec2> mTextureCoords;
};

GLuint shaderProgramID;
ModelData mesh_data;
GLuint vao = 0;
GLuint loc1, loc2;
GLfloat rotate_y = 0.0f;
int width = 800, height = 600;

// --------------------------------------------------
// Mesh loading (Assimp)
// --------------------------------------------------
ModelData load_mesh(const char* file_name) {
    ModelData modelData;
    const aiScene* scene = aiImportFile(
            file_name,
            aiProcess_Triangulate | aiProcess_PreTransformVertices
    );

    if (!scene) {
        fprintf(stderr, "ERROR: reading mesh %s\n", file_name);
        return modelData;
    }

    printf("Loaded: %i meshes\n", scene->mNumMeshes);

    for (unsigned int m_i = 0; m_i < scene->mNumMeshes; m_i++) {
        const aiMesh* mesh = scene->mMeshes[m_i];
        modelData.mPointCount += mesh->mNumVertices;
        for (unsigned int v_i = 0; v_i < mesh->mNumVertices; v_i++) {
            if (mesh->HasPositions()) {
                const aiVector3D* vp = &(mesh->mVertices[v_i]);
                modelData.mVertices.push_back(vec3(vp->x, vp->y, vp->z));
            }
            if (mesh->HasNormals()) {
                const aiVector3D* vn = &(mesh->mNormals[v_i]);
                modelData.mNormals.push_back(vec3(vn->x, vn->y, vn->z));
            }
            if (mesh->HasTextureCoords(0)) {
                const aiVector3D* vt = &(mesh->mTextureCoords[0][v_i]);
                modelData.mTextureCoords.push_back(vec2(vt->x, vt->y));
            }
        }
    }

    aiReleaseImport(scene);
    return modelData;
}

// --------------------------------------------------
// Shader helpers
// --------------------------------------------------
char* readShaderSource(const char* shaderFile) {
    FILE* fp = fopen(shaderFile, "rb");
    if (!fp) return nullptr;
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char* buf = new char[size + 1];
    fread(buf, 1, size, fp);
    buf[size] = '\0';
    fclose(fp);
    return buf;
}

static void AddShader(GLuint program, const char* file, GLenum type) {
    char* src = readShaderSource(file);
    GLuint sh = glCreateShader(type);
    glShaderSource(sh, 1, &src, nullptr);
    glCompileShader(sh);
    GLint success;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar log[1024];
        glGetShaderInfoLog(sh, 1024, nullptr, log);
        std::cerr << "Shader compile error (" << file << "): " << log << std::endl;
        exit(1);
    }
    glAttachShader(program, sh);
    delete[] src;
}

GLuint CompileShaders() {
    GLuint program = glCreateProgram();
    AddShader(program, "simpleVertexShader.txt", GL_VERTEX_SHADER);
    AddShader(program, "simpleFragmentShader.txt", GL_FRAGMENT_SHADER);

    glLinkProgram(program);
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar log[1024];
        glGetProgramInfoLog(program, 1024, nullptr, log);
        std::cerr << "Link error: " << log << std::endl;
        exit(1);
    }

    glUseProgram(program);
    return program;
}

// --------------------------------------------------
// Buffer setup
// --------------------------------------------------
void generateObjectBufferMesh() {
    mesh_data = load_mesh(MESH_NAME);

    GLuint vp_vbo, vn_vbo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vp_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vp_vbo);
    glBufferData(GL_ARRAY_BUFFER, mesh_data.mVertices.size() * sizeof(vec3), mesh_data.mVertices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &vn_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vn_vbo);
    glBufferData(GL_ARRAY_BUFFER, mesh_data.mNormals.size() * sizeof(vec3), mesh_data.mNormals.data(), GL_STATIC_DRAW);

    loc1 = glGetAttribLocation(shaderProgramID, "vertex_position");
    loc2 = glGetAttribLocation(shaderProgramID, "vertex_normal");

    glEnableVertexAttribArray(loc1);
    glBindBuffer(GL_ARRAY_BUFFER, vp_vbo);
    glVertexAttribPointer(loc1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    glEnableVertexAttribArray(loc2);
    glBindBuffer(GL_ARRAY_BUFFER, vn_vbo);
    glVertexAttribPointer(loc2, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    glBindVertexArray(0);
}

// --------------------------------------------------
// Render
// --------------------------------------------------
void drawScene(float delta) {
    rotate_y = fmodf(rotate_y + 20.0f * delta, 360.0f);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgramID);
    glBindVertexArray(vao);

    int model_loc = glGetUniformLocation(shaderProgramID, "model");
    int view_loc  = glGetUniformLocation(shaderProgramID, "view");
    int proj_loc  = glGetUniformLocation(shaderProgramID, "proj");

    mat4 view = translate(identity_mat4(), vec3(0.0f, 0.0f, -10.0f));
    mat4 proj = perspective(45.0f, (float)width / height, 0.1f, 100.0f);
    mat4 model = rotate_z_deg(identity_mat4(), rotate_y);

    glUniformMatrix4fv(proj_loc, 1, GL_FALSE, proj.m);
    glUniformMatrix4fv(view_loc, 1, GL_FALSE, view.m);
    glUniformMatrix4fv(model_loc, 1, GL_FALSE, model.m);

    glDrawArrays(GL_TRIANGLES, 0, mesh_data.mPointCount);
}

// --------------------------------------------------
// Main (GLFW loop)
// --------------------------------------------------
int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(width, height, "Lab 4", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    if (!gladLoadGL(glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD (GLAD2)\n";
        return -1;
    }

    shaderProgramID = CompileShaders();
    generateObjectBufferMesh();

    float lastTime = (float)glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        float now = (float)glfwGetTime();
        float delta = now - lastTime;
        lastTime = now;

        drawScene(delta);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
