#include "sphere_generator.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

SphereData generateSphere(float radius, unsigned int rings, unsigned int sectors) {
    SphereData sphere;

    float const R = 1.0f / (float)(rings - 1);
    float const S = 1.0f / (float)(sectors - 1);

    // Generate vertices, normals, and texture coordinates
    for(unsigned int r = 0; r < rings; r++) {
        for(unsigned int s = 0; s < sectors; s++) {
            float const y = sin(-M_PI / 2.0f + M_PI * r * R);
            float const x = cos(2.0f * M_PI * s * S) * sin(M_PI * r * R);
            float const z = sin(2.0f * M_PI * s * S) * sin(M_PI * r * R);

            // Texture coordinates
            float u = s * S;
            float v = r * R;
            sphere.texCoords.push_back(vec2(u, v));

            // Vertex position
            sphere.vertices.push_back(vec3(x * radius, y * radius, z * radius));

            // Normal (for a sphere, the normalized position is the normal)
            sphere.normals.push_back(vec3(x, y, z));
        }
    }

    // Generate indices for triangles
    for(unsigned int r = 0; r < rings - 1; r++) {
        for(unsigned int s = 0; s < sectors - 1; s++) {
            unsigned int curRow = r * sectors;
            unsigned int nextRow = (r + 1) * sectors;

            // First triangle
            sphere.indices.push_back(curRow + s);
            sphere.indices.push_back(nextRow + s);
            sphere.indices.push_back(nextRow + s + 1);

            // Second triangle
            sphere.indices.push_back(curRow + s);
            sphere.indices.push_back(nextRow + s + 1);
            sphere.indices.push_back(curRow + s + 1);
        }
    }

    sphere.indexCount = sphere.indices.size();

    return sphere;
}
