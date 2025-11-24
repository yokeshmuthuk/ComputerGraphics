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
GLuint loadCubemapFromFiles() {
    const char* faceNames[6] = {
        "skybox/right",   // +X
        "skybox/left",    // -X
        "skybox/top",     // +Y
        "skybox/bottom",  // -Y
        "skybox/front",   // +Z
        "skybox/back"     // -Z
    };

    const char* extensions[2] = {".png", ".jpg"};

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    bool allLoaded = true;
    for (unsigned int i = 0; i < 6; i++) {
        bool faceLoaded = false;

        // Try both .png and .jpg extensions
        for (int ext = 0; ext < 2; ext++) {
            char filepath[256];
            snprintf(filepath, sizeof(filepath), "%s%s", faceNames[i], extensions[ext]);

            int width, height, nrChannels;
            unsigned char* data = stbi_load(filepath, &width, &height, &nrChannels, 0);

            if (data) {
                GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format,
                             width, height, 0, format, GL_UNSIGNED_BYTE, data);
                stbi_image_free(data);
                printf("✓ Loaded skybox face: %s (%dx%d)\n", filepath, width, height);
                faceLoaded = true;
                break;  // Successfully loaded, move to next face
            }
        }

        if (!faceLoaded) {
            printf("✗ Failed to load skybox face: %s (.png or .jpg)\n", faceNames[i]);
            allLoaded = false;
            break;
        }
    }

    if (!allLoaded) {
        glDeleteTextures(1, &textureID);
        return 0;  // Return 0 to indicate failure
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    printf("✓ Loaded cubemap skybox from image files\n");
    return textureID;
}

GLuint createGalaxyCubemap() {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    const int size = 1024;  // Resolution per face
    srand(12345);  // Fixed seed for consistent stars across runs

    // Create procedural galaxy/starfield for each face
    for (unsigned int face = 0; face < 6; face++) {
        unsigned char* data = new unsigned char[size * size * 3];

        // Initialize to pure black space
        for (int i = 0; i < size * size * 3; i++) {
            data[i] = 0;
        }

        // Add stars
        int numStars = 2000 + (rand() % 1000);  // 2000-3000 stars per face
        for (int s = 0; s < numStars; s++) {
            int x = rand() % size;
            int y = rand() % size;
            int idx = (y * size + x) * 3;

            // Star brightness and color
            float brightness = 0.5f + ((float)(rand() % 100) / 100.0f) * 0.5f;  // 0.5 to 1.0

            // Slight color variation (bluish, white, or slightly yellow)
            float colorTint = (float)(rand() % 100) / 100.0f;
            unsigned char r, g, b;

            if (colorTint < 0.7f) {
                // White stars (most common)
                r = g = b = (unsigned char)(brightness * 255);
            }
            else if (colorTint < 0.85f) {
                // Bluish stars
                r = (unsigned char)(brightness * 200);
                g = (unsigned char)(brightness * 220);
                b = (unsigned char)(brightness * 255);
            }
            else {
                // Yellowish stars
                r = (unsigned char)(brightness * 255);
                g = (unsigned char)(brightness * 240);
                b = (unsigned char)(brightness * 200);
            }

            data[idx + 0] = r;
            data[idx + 1] = g;
            data[idx + 2] = b;

            // Add glow for brighter stars
            if (brightness > 0.8f) {
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        if (dx == 0 && dy == 0) continue;
                        int nx = x + dx;
                        int ny = y + dy;
                        if (nx >= 0 && nx < size && ny >= 0 && ny < size) {
                            int nidx = (ny * size + nx) * 3;
                            unsigned char glow = (unsigned char)(brightness * 60);
                            data[nidx + 0] = std::max(data[nidx + 0], (unsigned char)(r * 0.3f));
                            data[nidx + 1] = std::max(data[nidx + 1], (unsigned char)(g * 0.3f));
                            data[nidx + 2] = std::max(data[nidx + 2], (unsigned char)(b * 0.3f));
                        }
                    }
                }
            }
        }

        // Add very subtle nebula wisps (optional - much more subtle)
        for (int y = 0; y < size; y += 4) {
            for (int x = 0; x < size; x += 4) {
                float u = (float)x / size;
                float v = (float)y / size;

                // Very subtle nebula calculation
                float noise = sin(u * 50.0f + face * 1.5f) * cos(v * 40.0f + face * 2.0f);
                noise = noise * 0.5f + 0.5f;  // Normalize to 0-1

                if (noise > 0.85f) {  // Very high threshold
                    int idx = (y * size + x) * 3;
                    unsigned char tint = (unsigned char)((noise - 0.85f) * 40);  // Very subtle
                    data[idx + 0] = std::min(255, data[idx + 0] + tint / 3);      // Slight red
                    data[idx + 1] = std::min(255, data[idx + 1] + tint / 4);      // Less green
                    data[idx + 2] = std::min(255, data[idx + 2] + tint);          // More blue
                }
            }
        }

        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_RGB,
                     size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

        delete[] data;
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    printf("✓ Created procedural galaxy skybox with realistic starfield\n");
    return textureID;
}

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
    AddShader(program, "skyboxFragmentShader.txt", GL_FRAGMENT_SHADER);

    glLinkProgram(program);
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar log[1024];
        glGetProgramInfoLog(program, 1024, nullptr, log);
        std::cerr << "Skybox shader link error: " << log << std::endl;
        exit(1);
    }

    printf("✓ Skybox shaders compiled\n");
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

    // Draw skybox (render last with depth = 1.0)
    glDepthFunc(GL_LEQUAL);
    glUseProgram(skyboxShaderID);

    glUniformMatrix4fv(glGetUniformLocation(skyboxShaderID, "view"), 1, GL_FALSE, view.m);
    glUniformMatrix4fv(glGetUniformLocation(skyboxShaderID, "proj"), 1, GL_FALSE, proj.m);

    glBindVertexArray(skyboxVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
    glUniform1i(glGetUniformLocation(skyboxShaderID, "skybox"), 0);

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
    printf("\n=== Creating Galaxy Skybox ===\n");
    skyboxShaderID = CompileSkyboxShaders();
    setupSkybox();

    // Try to load skybox from image files first
    printf("Attempting to load skybox from image files...\n");
    skyboxTexture = loadCubemapFromFiles();
    if (skyboxTexture == 0) {
        printf("Skybox images not found - using procedural generation as fallback\n");
        skyboxTexture = createGalaxyCubemap();
    }

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
