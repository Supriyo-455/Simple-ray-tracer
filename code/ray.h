#pragma once

#include <stdint.h>

#include "ray_math.h"

struct image
{
    uint32_t Width;
    uint32_t Height;
    uint32_t* Pixels;
};

struct material
{
    vec3 EmitColor;
    vec3 ReflectionColor;
    float Scatter;     // NOTE: 0 is pure diffuse, 1 is pure specular
};

struct plane
{
    vec3 N;
    float d;
    uint32_t MatIndex;
};

struct sphere
{
    vec3 P;
    float r;
    uint32_t MatIndex;
};

struct world
{
    uint32_t MaterialCount;
    material* Materials;
    
    uint32_t PlaneCount;
    plane* Planes;
    
    uint32_t SpheresCount;
    sphere* Spheres;
};