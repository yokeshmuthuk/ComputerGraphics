#ifndef SPHERE_GENERATOR_H
#define SPHERE_GENERATOR_H

#include "maths_funcs.h"
#include <vector>
#include <cmath>

struct SphereData {
    std::vector<vec3> vertices;
    std::vector<vec3> normals;
    std::vector<vec2> texCoords;
    std::vector<unsigned int> indices;
    size_t indexCount;
};

// Generate a UV sphere with specified resolution
SphereData generateSphere(float radius, unsigned int rings, unsigned int sectors);

#endif // SPHERE_GENERATOR_H
