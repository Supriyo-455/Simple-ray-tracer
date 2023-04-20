#pragma once

#include "ray_math.h"
#include "ImageUtil.h"

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
    uint32 MatIndex;
};

struct sphere
{
    vec3 P;
    float r;
    uint32 MatIndex;
};

struct world
{
    uint32 MaterialCount;
    material* Materials;

    uint32 PlaneCount;
    plane* Planes;

    uint32 SpheresCount;
    sphere* Spheres;
};

struct work_order
{
    world* World;
    image* Image;
    uint32 XMin;
    uint32 YMin;
    uint32 OnePastXMax;
    uint32 OnePastYMax;
};

struct work_queue
{
    uint32 WorkOrderCount;
    work_order* WorkOrders;
    
    volatile uint32 NextWorkOrderIndex;
    volatile uint64 BouncesComputed;
    volatile uint32 TilesRetired;
};