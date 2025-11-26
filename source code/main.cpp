// Define math constants for Windows (MSVC)
#define _USE_MATH_DEFINES
#include <cmath>

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <iostream>
#include "maths_funcs.h"
#include "sphere_generator.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Assimp for model loading (optional)
#ifdef HAVE_ASSIMP
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#endif

#define EARTH_TEXTURE "earth_color.jpg"

// Global variables
GLuint shaderProgramID;
SphereData earth_sphere;
GLuint vao = 0;
GLuint earthTexture = 0;
GLuint ebo = 0;  // Element buffer object for indices
GLfloat rotate_y = 0.0f;

// 4K Resolution
int width = 3840, height = 2160;

// Camera variables
vec3 cameraPos = vec3(0.0f, 0.0f, 7.0f);
vec3 cameraFront = vec3(0.0f, 0.0f, -1.0f);
vec3 cameraUp = vec3(0.0f, 1.0f, 0.0f);
float cameraSpeed = 5.0f;

// Mouse control (click and drag)
float yaw = -90.0f;
float pitch = 0.0f;
float lastX = 0.0f;
float lastY = 0.0f;
bool isDragging = false;
float sensitivity = 0.3f;

// Skybox variables
GLuint skyboxVAO = 0;
GLuint skyboxVBO = 0;
GLuint skyboxTexture = 0;
GLuint skyboxShaderID = 0;

// Orbital Ring variables
GLuint ringVAO = 0;
GLuint ringVBO = 0;
GLuint ringEBO = 0;
GLuint ringShaderID = 0;
GLuint ringVertexCount = 0;
GLuint ringIndexCount = 0;
GLuint ringAlbedoTex = 0;
GLuint ringRoughnessTex = 0;
GLuint ringMetallicTex = 0;
GLuint ringAOTex = 0;
GLuint ringNormalTex = 0;
GLuint ringEmissiveTex = 0;
bool hasRingModel = false;
float ring_rotate_y = 0.0f;

// Turret system variables
const int NUM_TURRETS = 6;
struct Turret {
    vec3 position;
    float rotationY;  // Rotation around Y axis to face target
    float rotationX;  // Pitch to aim at target
    int targetCometIndex;  // Which comet is this turret tracking
    bool isFiring;
};
Turret turrets[NUM_TURRETS];
GLuint turretVAO = 0;
GLuint turretVBO = 0;
GLuint turretEBO = 0;
GLuint turretIndexCount = 0;
bool hasTurretModel = false;

// Comet system variables
const int MAX_COMETS = 10;
struct Comet {
    vec3 position;
    vec3 velocity;
    float scale;
    bool active;
};
Comet comets[MAX_COMETS];
float cometSpawnTimer = 0.0f;
const float COMET_SPAWN_INTERVAL = 3.0f;  // Spawn every 3 seconds

// --------------------------------------------------
// Texture loading
// --------------------------------------------------
GLuint loadTexture(const char* filename) {
    GLuint textureID;
    glGenTextures(1, &textureID);

    int width, height, nrChannels;
    unsigned char* data = stbi_load(filename, &width, &height, &nrChannels, 0);

    if (data) {
        GLenum format = GL_RGB;
        if (nrChannels == 1)
            format = GL_RED;
        else if (nrChannels == 3)
            format = GL_RGB;
        else if (nrChannels == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        // Set texture wrapping and filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
        printf("✓ Loaded texture: %s (%dx%d, %d channels)\n", filename, width, height, nrChannels);
    } else {
        std::cerr << "✗ Failed to load texture: " << filename << std::endl;
        std::cerr << "  Creating fallback procedural texture..." << std::endl;

        // Create a simple blue/green Earth-like procedural texture
        const int size = 512;
        unsigned char* proceduralData = new unsigned char[size * size * 3];
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                int idx = (y * size + x) * 3;
                float u = (float)x / size;
                float v = (float)y / size;
                // Simple blue/green pattern
                proceduralData[idx + 0] = (unsigned char)(30 + 50 * sin(u * 20.0f));  // R
                proceduralData[idx + 1] = (unsigned char)(100 + 50 * cos(v * 15.0f)); // G
                proceduralData[idx + 2] = (unsigned char)(150 + 50 * sin(u * v * 30.0f)); // B
            }
        }

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, proceduralData);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        delete[] proceduralData;
        printf("✓ Created procedural fallback texture\n");
    }

    return textureID;
}

// --------------------------------------------------
// Skybox / Galaxy creation
// --------------------------------------------------

// Load a single equirectangular HDR image
GLuint loadEquirectangularHDR(const char* filepath) {
    stbi_set_flip_vertically_on_load(true);
    int width, height, nrChannels;
    float* data = stbi_loadf(filepath, &width, &height, &nrChannels, 0);

    if (!data) {
        printf("✗ Failed to load equirectangular HDR: %s\n", filepath);
        return 0;
    }

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    GLenum internalFormat = (nrChannels == 4) ? GL_RGBA16F : GL_RGB16F;
    GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;

    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_FLOAT, data);
    stbi_image_free(data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    printf("✓ Loaded equirectangular HDR: %s (%dx%d, HDR)\n", filepath, width, height);
    return textureID;
}

// Only equirectangular HDR support - cubemap and procedural functions removed

void setupSkybox() {
    float skyboxVertices[] = {
        // positions
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };

    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);

    printf("✓ Skybox mesh created\n");
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

GLuint CompileSkyboxShaders() {
    GLuint program = glCreateProgram();
    AddShader(program, "skyboxVertexShader.txt", GL_VERTEX_SHADER);
    AddShader(program, "skyboxEquirectFragmentShader.txt", GL_FRAGMENT_SHADER);

    glLinkProgram(program);
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar log[1024];
        glGetProgramInfoLog(program, 1024, nullptr, log);
        std::cerr << "Equirectangular HDR shader link error: " << log << std::endl;
        exit(1);
    }

    printf("✓ Equirectangular HDR skybox shaders compiled\n");
    return program;
}

GLuint CompilePBRShaders() {
    GLuint program = glCreateProgram();
    AddShader(program, "pbrVertexShader.txt", GL_VERTEX_SHADER);
    AddShader(program, "pbrFragmentShader.txt", GL_FRAGMENT_SHADER);

    glLinkProgram(program);
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar log[1024];
        glGetProgramInfoLog(program, 1024, nullptr, log);
        std::cerr << "PBR shader link error: " << log << std::endl;
        exit(1);
    }

    printf("✓ PBR shaders compiled\n");
    return program;
}

// --------------------------------------------------
// Buffer setup
// --------------------------------------------------
void generateEarthSphere() {
    // Generate sphere geometry (radius, rings, sectors)
    earth_sphere = generateSphere(2.0f, 64, 64);
    printf("Generated Earth sphere: %zu vertices, %zu indices\n",
           earth_sphere.vertices.size(), earth_sphere.indexCount);

    GLuint vp_vbo, vn_vbo, vt_vbo;

    // Create VAO
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Vertex positions
    glGenBuffers(1, &vp_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vp_vbo);
    glBufferData(GL_ARRAY_BUFFER, earth_sphere.vertices.size() * sizeof(vec3),
                 earth_sphere.vertices.data(), GL_STATIC_DRAW);
    GLuint loc_pos = glGetAttribLocation(shaderProgramID, "vertex_position");
    glEnableVertexAttribArray(loc_pos);
    glVertexAttribPointer(loc_pos, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    // Vertex normals
    glGenBuffers(1, &vn_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vn_vbo);
    glBufferData(GL_ARRAY_BUFFER, earth_sphere.normals.size() * sizeof(vec3),
                 earth_sphere.normals.data(), GL_STATIC_DRAW);
    GLuint loc_norm = glGetAttribLocation(shaderProgramID, "vertex_normal");
    glEnableVertexAttribArray(loc_norm);
    glVertexAttribPointer(loc_norm, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    // Texture coordinates
    glGenBuffers(1, &vt_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vt_vbo);
    glBufferData(GL_ARRAY_BUFFER, earth_sphere.texCoords.size() * sizeof(vec2),
                 earth_sphere.texCoords.data(), GL_STATIC_DRAW);
    GLuint loc_tex = glGetAttribLocation(shaderProgramID, "vertex_texcoord");
    glEnableVertexAttribArray(loc_tex);
    glVertexAttribPointer(loc_tex, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    // Element buffer for indices
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, earth_sphere.indices.size() * sizeof(unsigned int),
                 earth_sphere.indices.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);
}

void loadOrbitalRing() {
    printf("\n=== Loading Orbital Ring Model ===\n");

#ifdef HAVE_ASSIMP
    // Try loading the model
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile("orbital_ring/model.obj",
        aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        printf("✗ Failed to load orbital ring model: %s\n", importer.GetErrorString());
        hasRingModel = false;
        return;
    }

    // Assume first mesh
    if (scene->mNumMeshes == 0) {
        printf("✗ No meshes found in model\n");
        hasRingModel = false;
        return;
    }

    aiMesh* mesh = scene->mMeshes[0];
    printf("✓ Loaded model: %d vertices, %d faces\n", mesh->mNumVertices, mesh->mNumFaces);

    // Extract vertices, normals, texcoords, tangents
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        // Position
        vertices.push_back(mesh->mVertices[i].x);
        vertices.push_back(mesh->mVertices[i].y);
        vertices.push_back(mesh->mVertices[i].z);

        // Normal
        vertices.push_back(mesh->mNormals[i].x);
        vertices.push_back(mesh->mNormals[i].y);
        vertices.push_back(mesh->mNormals[i].z);

        // TexCoords
        if (mesh->mTextureCoords[0]) {
            vertices.push_back(mesh->mTextureCoords[0][i].x);
            vertices.push_back(mesh->mTextureCoords[0][i].y);
        } else {
            vertices.push_back(0.0f);
            vertices.push_back(0.0f);
        }

        // Tangent
        if (mesh->mTangents) {
            vertices.push_back(mesh->mTangents[i].x);
            vertices.push_back(mesh->mTangents[i].y);
            vertices.push_back(mesh->mTangents[i].z);
        } else {
            vertices.push_back(1.0f);
            vertices.push_back(0.0f);
            vertices.push_back(0.0f);
        }
    }

    // Extract indices
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    ringVertexCount = mesh->mNumVertices;
    ringIndexCount = indices.size();

    // Create VAO/VBO/EBO
    glGenVertexArrays(1, &ringVAO);
    glGenBuffers(1, &ringVBO);
    glGenBuffers(1, &ringEBO);

    glBindVertexArray(ringVAO);

    glBindBuffer(GL_ARRAY_BUFFER, ringVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ringEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Vertex attributes: position(3) + normal(3) + texcoord(2) + tangent(3) = 11 floats
    int stride = 11 * sizeof(float);

    // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);

    // Normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));

    // TexCoord
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));

    // Tangent
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (void*)(8 * sizeof(float)));

    glBindVertexArray(0);

    // Load PBR textures
    printf("Loading PBR textures...\n");
    ringAlbedoTex = loadTexture("orbital_ring/albedo.png");
    if (ringAlbedoTex == 0) ringAlbedoTex = loadTexture("orbital_ring/albedo.jpg");

    ringRoughnessTex = loadTexture("orbital_ring/roughness.png");
    if (ringRoughnessTex == 0) ringRoughnessTex = loadTexture("orbital_ring/roughness.jpg");

    ringMetallicTex = loadTexture("orbital_ring/metallic.png");
    if (ringMetallicTex == 0) ringMetallicTex = loadTexture("orbital_ring/metallic.jpg");

    ringAOTex = loadTexture("orbital_ring/ao.png");
    if (ringAOTex == 0) ringAOTex = loadTexture("orbital_ring/ao.jpg");

    ringNormalTex = loadTexture("orbital_ring/normal.png");
    if (ringNormalTex == 0) ringNormalTex = loadTexture("orbital_ring/normal.jpg");

    ringEmissiveTex = loadTexture("orbital_ring/emissive.png");
    if (ringEmissiveTex == 0) ringEmissiveTex = loadTexture("orbital_ring/emissive.jpg");

    hasRingModel = true;
    printf("✓ Orbital ring model loaded successfully\n");
#else
    printf("✗ Assimp not available - orbital ring cannot be loaded\n");
    printf("  To enable orbital ring: install Assimp library\n");
    hasRingModel = false;
#endif
}

// --------------------------------------------------
// Turret Defense System
// --------------------------------------------------

void loadTurretModel() {
    printf("\n=== Loading Turret Model ===\n");
#ifdef HAVE_ASSIMP
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile("turret/model.obj",
        aiProcess_Triangulate | aiProcess_FlipUVs);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode || scene->mNumMeshes == 0) {
        printf("✗ Failed to load turret model - using fallback\n");
        hasTurretModel = false;
        return;
    }

    aiMesh* mesh = scene->mMeshes[0];
    printf("✓ Loaded turret: %d vertices, %d faces\n", mesh->mNumVertices, mesh->mNumFaces);

    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        vertices.push_back(mesh->mVertices[i].x);
        vertices.push_back(mesh->mVertices[i].y);
        vertices.push_back(mesh->mVertices[i].z);
        vertices.push_back(mesh->mNormals[i].x);
        vertices.push_back(mesh->mNormals[i].y);
        vertices.push_back(mesh->mNormals[i].z);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    turretIndexCount = indices.size();

    glGenVertexArrays(1, &turretVAO);
    glGenBuffers(1, &turretVBO);
    glGenBuffers(1, &turretEBO);

    glBindVertexArray(turretVAO);
    glBindBuffer(GL_ARRAY_BUFFER, turretVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, turretEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindVertexArray(0);
    hasTurretModel = true;
#else
    printf("✗ Assimp not available - cannot load turret\n");
    hasTurretModel = false;
#endif
}

void initializeTurrets() {
    float ringRadius = 1.5f;  // Position turrets at proper distance on the ring
    for (int i = 0; i < NUM_TURRETS; i++) {
        float angle = (360.0f / NUM_TURRETS) * i;
        float rad = angle * 3.14159f / 180.0f;
        turrets[i].position = vec3(cos(rad) * ringRadius, 0.0f, sin(rad) * ringRadius);
        turrets[i].rotationY = angle;
        turrets[i].rotationX = 0.0f;
        turrets[i].targetCometIndex = -1;
        turrets[i].isFiring = false;
    }
    printf("✓ Initialized %d turrets around ring at radius %.2f\n", NUM_TURRETS, ringRadius);
}

void spawnComet() {
    for (int i = 0; i < MAX_COMETS; i++) {
        if (!comets[i].active) {
            // Random position far from Earth
            float angle1 = (rand() % 360) * 3.14159f / 180.0f;
            float angle2 = (rand() % 360) * 3.14159f / 180.0f;
            float distance = 15.0f + (rand() % 10);

            comets[i].position = vec3(
                sin(angle1) * cos(angle2) * distance,
                sin(angle2) * distance,
                cos(angle1) * cos(angle2) * distance
            );

            // Velocity towards Earth
            vec3 toEarth = normalise(vec3(0,0,0) - comets[i].position);
            float speed = 0.5f + (rand() % 100) / 200.0f;  // 0.5 to 1.0
            comets[i].velocity = toEarth * speed;
            comets[i].scale = 0.2f + (rand() % 100) / 500.0f;
            comets[i].active = true;
            break;
        }
    }
}

void updateComets(float deltaTime) {
    for (int i = 0; i < MAX_COMETS; i++) {
        if (comets[i].active) {
            comets[i].position = comets[i].position + comets[i].velocity * deltaTime;

            // Deactivate if too close to Earth or too far
            float dist = length(comets[i].position);
            if (dist < 1.5f || dist > 30.0f) {
                comets[i].active = false;
            }
        }
    }
}

void updateTurrets(float deltaTime) {
    for (int i = 0; i < NUM_TURRETS; i++) {
        // Find nearest comet
        float nearestDist = 999999.0f;
        int nearestIndex = -1;

        for (int j = 0; j < MAX_COMETS; j++) {
            if (comets[j].active) {
                float dist = length(comets[j].position - turrets[i].position);
                if (dist < nearestDist && dist < 20.0f) {  // 20 unit range
                    nearestDist = dist;
                    nearestIndex = j;
                }
            }
        }

        turrets[i].targetCometIndex = nearestIndex;

        if (nearestIndex >= 0) {
            // Calculate direction to target
            vec3 toTarget = normalise(comets[nearestIndex].position - turrets[i].position);

            // Calculate rotation angles
            turrets[i].rotationY = atan2(toTarget.v[0], toTarget.v[2]) * 180.0f / 3.14159f;
            turrets[i].rotationX = -asin(toTarget.v[1]) * 180.0f / 3.14159f;

            // Fire only if in range
            turrets[i].isFiring = (nearestDist < 12.0f);  // Fire within 12 units

            // Instant destruction when laser hits
            if (turrets[i].isFiring) {
                comets[nearestIndex].active = false;  // Destroyed instantly!
                printf("Comet destroyed by turret %d!\n", i);
            }
        } else {
            turrets[i].isFiring = false;
        }
    }
}

void renderLasers(mat4 view, mat4 proj) {
    glUseProgram(shaderProgramID);

    // Enable blending for neon glow effect
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);  // Additive blending for glow
    glDisable(GL_DEPTH_TEST);  // Render lasers on top

    // Set view and projection matrices
    glUniformMatrix4fv(glGetUniformLocation(shaderProgramID, "view"), 1, GL_FALSE, view.m);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgramID, "proj"), 1, GL_FALSE, proj.m);

    for (int i = 0; i < NUM_TURRETS; i++) {
        if (turrets[i].isFiring && turrets[i].targetCometIndex >= 0) {
            int targetIdx = turrets[i].targetCometIndex;
            if (comets[targetIdx].active) {
                // Create line geometry for laser
                float laserVerts[] = {
                    turrets[i].position.v[0], turrets[i].position.v[1], turrets[i].position.v[2],
                    comets[targetIdx].position.v[0], comets[targetIdx].position.v[1], comets[targetIdx].position.v[2]
                };

                GLuint laserVBO, laserVAO;
                glGenVertexArrays(1, &laserVAO);
                glGenBuffers(1, &laserVBO);

                glBindVertexArray(laserVAO);
                glBindBuffer(GL_ARRAY_BUFFER, laserVBO);
                glBufferData(GL_ARRAY_BUFFER, sizeof(laserVerts), laserVerts, GL_STATIC_DRAW);

                glEnableVertexAttribArray(0);
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

                // Identity model matrix for lasers (already in world space)
                mat4 laserModel = identity_mat4();
                glUniformMatrix4fv(glGetUniformLocation(shaderProgramID, "model"), 1, GL_FALSE, laserModel.m);

                // Render outer glow (thicker, dimmer)
                glLineWidth(10.0f);
                glUniform3f(glGetUniformLocation(shaderProgramID, "objectColor"), 0.0f, 3.0f, 0.0f);
                glDrawArrays(GL_LINES, 0, 2);

                // Render inner core (thinner, brighter)
                glLineWidth(3.0f);
                glUniform3f(glGetUniformLocation(shaderProgramID, "objectColor"), 0.5f, 8.0f, 0.5f);
                glDrawArrays(GL_LINES, 0, 2);

                glDeleteBuffers(1, &laserVBO);
                glDeleteVertexArrays(1, &laserVAO);
            }
        }
    }

    // Restore normal rendering state
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

// --------------------------------------------------
// Input Callbacks
// --------------------------------------------------
void processInput(GLFWwindow* window, float deltaTime) {
    // Close window on ESC
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float velocity = cameraSpeed * deltaTime;

    // WASD movement
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos = cameraPos + (cameraFront * velocity);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos = cameraPos - (cameraFront * velocity);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos = cameraPos - (normalise(cross(cameraFront, cameraUp)) * velocity);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos = cameraPos + (normalise(cross(cameraFront, cameraUp)) * velocity);

    // Q/E for up/down
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        cameraPos = cameraPos - (cameraUp * velocity);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        cameraPos = cameraPos + (cameraUp * velocity);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if ((button == GLFW_MOUSE_BUTTON_LEFT || button == GLFW_MOUSE_BUTTON_RIGHT) && action == GLFW_PRESS) {
        isDragging = true;
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        lastX = (float)xpos;
        lastY = (float)ypos;
    }
    else if ((button == GLFW_MOUSE_BUTTON_LEFT || button == GLFW_MOUSE_BUTTON_RIGHT) && action == GLFW_RELEASE) {
        isDragging = false;
    }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (!isDragging) return;  // Only rotate when dragging

    float xposf = (float)xpos;
    float yposf = (float)ypos;

    float xoffset = xposf - lastX;
    float yoffset = lastY - yposf; // Reversed: y-coordinates go from bottom to top

    lastX = xposf;
    lastY = yposf;

    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    // Constrain pitch
    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    // Update camera front vector
    vec3 front;
    front.v[0] = cos(yaw * ONE_DEG_IN_RAD) * cos(pitch * ONE_DEG_IN_RAD);
    front.v[1] = sin(pitch * ONE_DEG_IN_RAD);
    front.v[2] = sin(yaw * ONE_DEG_IN_RAD) * cos(pitch * ONE_DEG_IN_RAD);
    cameraFront = normalise(front);
}

// --------------------------------------------------
// Render
// --------------------------------------------------
void drawScene(float delta, GLFWwindow* window) {
    // Process keyboard/mouse input
    processInput(window, delta);

    // Update rotation (Earth spins)
    rotate_y = fmodf(rotate_y + 15.0f * delta, 360.0f);

    // BLACK SPACE background
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);  // Black background for space
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgramID);
    glBindVertexArray(vao);

    // Bind Earth texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, earthTexture);
    glUniform1i(glGetUniformLocation(shaderProgramID, "earthTexture"), 0);

    // Set up transformation matrices
    int model_loc = glGetUniformLocation(shaderProgramID, "model");
    int view_loc  = glGetUniformLocation(shaderProgramID, "view");
    int proj_loc  = glGetUniformLocation(shaderProgramID, "proj");

    // Use camera position and look-at direction
    vec3 center = cameraPos + cameraFront;
    mat4 view = look_at(cameraPos, center, cameraUp);
    mat4 proj = perspective(45.0f, (float)width / height, 0.1f, 1000.0f);

    // Earth rotation (around Y axis for proper rotation)
    mat4 model = rotate_y_deg(identity_mat4(), rotate_y);

    glUniformMatrix4fv(proj_loc, 1, GL_FALSE, proj.m);
    glUniformMatrix4fv(view_loc, 1, GL_FALSE, view.m);
    glUniformMatrix4fv(model_loc, 1, GL_FALSE, model.m);

    // Advanced lighting uniforms
    vec3 lightPos = vec3(10.0f, 10.0f, 10.0f);  // Light position
    vec3 lightColor = vec3(1.0f, 1.0f, 1.0f);   // White light

    glUniform3f(glGetUniformLocation(shaderProgramID, "lightPos"),
                lightPos.v[0], lightPos.v[1], lightPos.v[2]);
    glUniform3f(glGetUniformLocation(shaderProgramID, "lightColor"),
                lightColor.v[0], lightColor.v[1], lightColor.v[2]);

    // Phong lighting parameters
    glUniform1f(glGetUniformLocation(shaderProgramID, "ambientStrength"), 0.2f);
    glUniform1f(glGetUniformLocation(shaderProgramID, "diffuseStrength"), 0.8f);
    glUniform1f(glGetUniformLocation(shaderProgramID, "specularStrength"), 0.3f);
    glUniform1f(glGetUniformLocation(shaderProgramID, "shininess"), 32.0f);

    // Draw Earth sphere using indexed rendering
    glDrawElements(GL_TRIANGLES, earth_sphere.indexCount, GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);

    // Draw orbital ring (if loaded)
    if (hasRingModel) {
        // Update ring rotation (slower than Earth)
        ring_rotate_y = fmodf(ring_rotate_y + 5.0f * delta, 360.0f);

        glUseProgram(ringShaderID);
        glBindVertexArray(ringVAO);

        // Ring transformation (horizontal, static, around Earth)
        mat4 ringModel = identity_mat4();
        ringModel = scale(ringModel, vec3(0.08f, 0.08f, 0.08f));  // Scale to fit around Earth
        ringModel = rotate_x_deg(ringModel, 90.0f);  // Rotate to lie flat (horizontal around equator)
        // No rotation animation - static ring

        glUniformMatrix4fv(glGetUniformLocation(ringShaderID, "model"), 1, GL_FALSE, ringModel.m);
        glUniformMatrix4fv(glGetUniformLocation(ringShaderID, "view"), 1, GL_FALSE, view.m);
        glUniformMatrix4fv(glGetUniformLocation(ringShaderID, "proj"), 1, GL_FALSE, proj.m);

        // Dedicated ring light - positioned close to ring for strong illumination
        vec3 ringLightPos = vec3(0.0f, 3.0f, 0.0f);  // Above the ring
        vec3 ringLightColor = vec3(8.0f, 8.0f, 8.0f);  // Very bright white light (8x intensity)

        glUniform3f(glGetUniformLocation(ringShaderID, "lightPos"), ringLightPos.v[0], ringLightPos.v[1], ringLightPos.v[2]);
        glUniform3f(glGetUniformLocation(ringShaderID, "viewPos"), cameraPos.v[0], cameraPos.v[1], cameraPos.v[2]);
        glUniform3f(glGetUniformLocation(ringShaderID, "lightColor"), ringLightColor.v[0], ringLightColor.v[1], ringLightColor.v[2]);

        // Bind PBR textures
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ringAlbedoTex);
        glUniform1i(glGetUniformLocation(ringShaderID, "albedoMap"), 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, ringRoughnessTex);
        glUniform1i(glGetUniformLocation(ringShaderID, "roughnessMap"), 1);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, ringMetallicTex);
        glUniform1i(glGetUniformLocation(ringShaderID, "metallicMap"), 2);

        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, ringAOTex);
        glUniform1i(glGetUniformLocation(ringShaderID, "aoMap"), 3);

        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, ringNormalTex);
        glUniform1i(glGetUniformLocation(ringShaderID, "normalMap"), 4);

        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, ringEmissiveTex);
        glUniform1i(glGetUniformLocation(ringShaderID, "emissiveMap"), 5);

        // Set texture availability flags
        glUniform1i(glGetUniformLocation(ringShaderID, "hasNormalMap"), ringNormalTex != 0);
        glUniform1i(glGetUniformLocation(ringShaderID, "hasEmissiveMap"), ringEmissiveTex != 0);

        // Draw ring
        glDrawElements(GL_TRIANGLES, ringIndexCount, GL_UNSIGNED_INT, 0);

        glBindVertexArray(0);
    }

    // Update and render turret defense system
    updateTurrets(delta);
    updateComets(delta);

    // Spawn new comets periodically
    cometSpawnTimer += delta;
    if (cometSpawnTimer >= COMET_SPAWN_INTERVAL) {
        spawnComet();
        cometSpawnTimer = 0.0f;
    }

    // Draw turrets
    if (hasTurretModel) {
        glUseProgram(shaderProgramID);
        for (int i = 0; i < NUM_TURRETS; i++) {
            mat4 turretModel = identity_mat4();
            turretModel = translate(turretModel, turrets[i].position);
            turretModel = rotate_y_deg(turretModel, turrets[i].rotationY);
            turretModel = rotate_x_deg(turretModel, turrets[i].rotationX);
            turretModel = scale(turretModel, vec3(0.1f, 0.1f, 0.1f));  // Bigger for visibility

            glUniformMatrix4fv(glGetUniformLocation(shaderProgramID, "model"), 1, GL_FALSE, turretModel.m);
            glUniformMatrix4fv(glGetUniformLocation(shaderProgramID, "view"), 1, GL_FALSE, view.m);
            glUniformMatrix4fv(glGetUniformLocation(shaderProgramID, "proj"), 1, GL_FALSE, proj.m);

            // Set lighting uniforms for turrets
            glUniform3f(glGetUniformLocation(shaderProgramID, "lightPos"), lightPos.v[0], lightPos.v[1], lightPos.v[2]);
            glUniform3f(glGetUniformLocation(shaderProgramID, "viewPos"), cameraPos.v[0], cameraPos.v[1], cameraPos.v[2]);
            glUniform3f(glGetUniformLocation(shaderProgramID, "lightColor"), lightColor.v[0], lightColor.v[1], lightColor.v[2]);

            // Bright metallic color for turrets (cyan when firing, bright gray otherwise)
            vec3 turretColor = turrets[i].isFiring ? vec3(0.0f, 2.0f, 2.0f) : vec3(1.5f, 1.5f, 1.5f);
            glUniform3f(glGetUniformLocation(shaderProgramID, "objectColor"), turretColor.v[0], turretColor.v[1], turretColor.v[2]);

            glBindVertexArray(turretVAO);
            glDrawElements(GL_TRIANGLES, turretIndexCount, GL_UNSIGNED_INT, 0);
        }
    }

    // Draw comets (unbind PBR textures first!)
    glUseProgram(shaderProgramID);

    // Unbind all texture units to prevent PBR texture bleed
    for (int texUnit = 0; texUnit < 6; texUnit++) {
        glActiveTexture(GL_TEXTURE0 + texUnit);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    for (int i = 0; i < MAX_COMETS; i++) {
        if (comets[i].active) {
            mat4 cometModel = identity_mat4();
            cometModel = translate(cometModel, comets[i].position);
            cometModel = scale(cometModel, vec3(comets[i].scale, comets[i].scale, comets[i].scale));

            glUniformMatrix4fv(glGetUniformLocation(shaderProgramID, "model"), 1, GL_FALSE, cometModel.m);
            glUniformMatrix4fv(glGetUniformLocation(shaderProgramID, "view"), 1, GL_FALSE, view.m);
            glUniformMatrix4fv(glGetUniformLocation(shaderProgramID, "proj"), 1, GL_FALSE, proj.m);

            // Set lighting (same as Earth)
            glUniform3f(glGetUniformLocation(shaderProgramID, "lightPos"), lightPos.v[0], lightPos.v[1], lightPos.v[2]);
            glUniform3f(glGetUniformLocation(shaderProgramID, "viewPos"), cameraPos.v[0], cameraPos.v[1], cameraPos.v[2]);
            glUniform3f(glGetUniformLocation(shaderProgramID, "lightColor"), lightColor.v[0], lightColor.v[1], lightColor.v[2]);

            // Bright orange/red color for comets (visible asteroids)
            glUniform3f(glGetUniformLocation(shaderProgramID, "objectColor"), 1.2f, 0.4f, 0.1f);

            glBindVertexArray(vao);  // Reuse Earth sphere for comets
            glDrawElements(GL_TRIANGLES, earth_sphere.indexCount, GL_UNSIGNED_INT, 0);
        }
    }

    // Draw laser beams
    renderLasers(view, proj);

    // Draw skybox (render last with depth = 1.0)
    glDepthFunc(GL_LEQUAL);
    glUseProgram(skyboxShaderID);

    glUniformMatrix4fv(glGetUniformLocation(skyboxShaderID, "view"), 1, GL_FALSE, view.m);
    glUniformMatrix4fv(glGetUniformLocation(skyboxShaderID, "proj"), 1, GL_FALSE, proj.m);

    // Set exposure for HDR tone mapping
    glUniform1f(glGetUniformLocation(skyboxShaderID, "exposure"), 1.0f);

    glBindVertexArray(skyboxVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, skyboxTexture);
    glUniform1i(glGetUniformLocation(skyboxShaderID, "equirectangularMap"), 0);

    glDrawArrays(GL_TRIANGLES, 0, 36);

    glBindVertexArray(0);
    glDepthFunc(GL_LESS);  // Reset to default
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

    GLFWwindow* window = glfwCreateWindow(width, height, "Earth - OpenGL 3D", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    if (!gladLoadGL(glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD (GLAD2)\n";
        return -1;
    }

    // Get actual framebuffer size (might differ from window size on high-DPI displays)
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);

    // Update width/height to actual framebuffer size for correct aspect ratio
    width = fbWidth;
    height = fbHeight;

    // Setup mouse input (click and drag)
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    // Cursor is visible and normal (not captured)

    shaderProgramID = CompileShaders();

    // Load Earth texture
    printf("\n=== Loading Earth Texture ===\n");
    earthTexture = loadTexture(EARTH_TEXTURE);

    // Generate Earth sphere geometry
    printf("\n=== Generating Earth Sphere ===\n");
    generateEarthSphere();

    // Setup galaxy skybox
    printf("\n=== Loading Equirectangular HDR Skybox ===\n");
    skyboxShaderID = CompileSkyboxShaders();
    setupSkybox();

    // Try loading equirectangular HDR
    const char* equirectFiles[] = {"skybox/environment.hdr", "skybox/skybox.hdr", "skybox/space.hdr"};
    for (int i = 0; i < 3; i++) {
        skyboxTexture = loadEquirectangularHDR(equirectFiles[i]);
        if (skyboxTexture != 0) {
            break;
        }
    }

    if (skyboxTexture == 0) {
        std::cerr << "\n✗ ERROR: No equirectangular HDR file found!" << std::endl;
        std::cerr << "Please place one of these files in 'source code/skybox/':" << std::endl;
        std::cerr << "  - environment.hdr" << std::endl;
        std::cerr << "  - skybox.hdr" << std::endl;
        std::cerr << "  - space.hdr" << std::endl;
        std::cerr << "\nDownload HDR skyboxes from: https://polyhaven.com/hdris" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Load orbital ring (optional)
    ringShaderID = CompilePBRShaders();
    loadOrbitalRing();

    // Initialize turret defense system
    loadTurretModel();
    initializeTurrets();

    // Initialize comets (all inactive at start)
    for (int i = 0; i < MAX_COMETS; i++) {
        comets[i].active = false;
    }
    printf("✓ Turret defense system initialized\n");

    printf("\n=== Starting Render Loop ===\n");
    printf("Controls:\n");
    printf("  WASD - Move camera\n");
    printf("  Q/E - Move up/down\n");
    printf("  Left/Right Click + Drag - Rotate view\n");
    printf("  ESC - Exit\n");
    printf("\nActual Resolution: %dx%d\n\n", width, height);

    float lastTime = (float)glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        float now = (float)glfwGetTime();
        float delta = now - lastTime;
        lastTime = now;

        drawScene(delta, window);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
