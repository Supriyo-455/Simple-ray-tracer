#include <stdio.h>
#include <assert.h>
#include <time.h>
#include "ray_math.h"
#include "ImageUtil.h"
#include "ray.h"

// TODO: Make Platform independent in future
#include <Windows.h>

internal uint32 LockedAddAndReturn(uint32 volatile* Value, uint32 Addend)
{
    uint32 Result = *Value;
    *Value += Addend;
    return Result;
}

internal void LockedAdd(uint32 volatile* Value, uint32 Addend)
{
    *Value += Addend;
}

internal void LockedAdd(uint64 volatile* Value, uint32 Addend)
{
    *Value += Addend;
}

internal vec3 RayCast(work_queue* Queue, world *World, vec3 RayOrigin, vec3 RayDirection)
{
    float MinHitDistance = 0.0001f;
    float Tolerance = 0.00001f;
    uint32 BouncesComputed = 0;

    vec3 Result = {};
    vec3 Attenuation = vec(1, 1, 1);
    for (uint32_t RayCount = 0; RayCount < 1000; ++RayCount)
    {
        float HitDistance = FLT_MAX;

        vec3 NextNormal = {};
        vec3 NextOrigin = {};
        uint32_t HitMaterialIndex = 0;

        ++BouncesComputed;
        for (uint32_t PlaneIndex = 0; PlaneIndex < World->PlaneCount; ++PlaneIndex)
        {
            plane Plane = World->Planes[PlaneIndex];

            float Denom = Inner(Plane.N, RayDirection);
            if (absf(Denom < -Tolerance))
            {
                float ThisDistance = (-Plane.d - Inner(Plane.N, RayOrigin)) / Denom;
                if ((ThisDistance > MinHitDistance) && (ThisDistance < HitDistance))
                {
                    HitDistance = ThisDistance;
                    HitMaterialIndex = Plane.MatIndex;
                    NextOrigin = RayOrigin + (RayDirection * ThisDistance);
                    NextNormal = Plane.N;
                }
            }
        }

        for (uint32_t SphereIndex = 0; SphereIndex < World->SpheresCount; ++SphereIndex)
        {
            sphere Sphere = World->Spheres[SphereIndex];

            vec3 SphereRelativeRayOrigin = RayOrigin - Sphere.P;
            float a = Inner(RayDirection, RayDirection);
            float b = 2.0f * Inner(RayDirection, SphereRelativeRayOrigin);
            float c = Inner(SphereRelativeRayOrigin, SphereRelativeRayOrigin) - Sphere.r * Sphere.r;

            float Denom = 2.0f * a;
            float RootTerm = sqrtf(b * b - 4.0f * a * c);
            if (RootTerm > Tolerance)
            {
                float tp = (-b + RootTerm) / Denom;
                float tn = (-b - RootTerm) / Denom;

                float ThisDistance = tp;
                if ((tp > MinHitDistance) && (tn < tp))
                {
                    ThisDistance = tn;
                }

                if ((ThisDistance > MinHitDistance) && (ThisDistance < HitDistance))
                {
                    HitDistance = ThisDistance;
                    HitMaterialIndex = Sphere.MatIndex;
                    NextOrigin = RayOrigin + (RayDirection * ThisDistance);
                    // NextNormal = NormalizeOrZero((RayDirection * ThisDistance) - SphereRelativeRayOrigin);
                    NextNormal = NormalizeOrZero(NextOrigin - Sphere.P);
                }
            }
        }

        if (HitMaterialIndex)
        {
            material Mat = World->Materials[HitMaterialIndex];

            Result = Result + Hadamard(Attenuation, Mat.EmitColor);
            float CosAtten = Inner(-RayDirection, NextNormal);
            if (CosAtten < 0)
            {
                CosAtten = 0.0f;
            }
            Attenuation = Hadamard(Attenuation, Mat.ReflectionColor * CosAtten);

            RayOrigin = NextOrigin;

            vec3 PureBounce = RayDirection - (NextNormal * (2.0f * Inner(RayDirection, NextNormal)));
            vec3 RandomBounce = NormalizeOrZero(NextNormal + vec(RandomBilateral(), RandomBilateral(), RandomBilateral()));
            RayDirection = NormalizeOrZero(Lerp(RandomBounce, Mat.Scatter, PureBounce));

            // RayDirection = PureBounce;
        }
        else
        {
            material Mat = World->Materials[HitMaterialIndex];
            Result = Result + Hadamard(Attenuation, Mat.EmitColor);
            break;
        }
    }

    LockedAdd(&Queue->BouncesComputed, BouncesComputed);
    return Result;
}
 
internal bool32 RenderTile(work_queue* Queue)
{

    uint32 WorkOrderIndex = LockedAddAndReturn(&Queue->NextWorkOrderIndex, 1);
    if (WorkOrderIndex >= Queue->WorkOrderCount)
    {
        return false;
    }
    work_order* WorkOrder = Queue->WorkOrders + WorkOrderIndex;

    vec3 CameraP = vec(0, -10, 1);
    vec3 CameraZ = NormalizeOrZero(CameraP);
    vec3 CameraX = NormalizeOrZero(Cross(vec(0, 0, 1), CameraZ));
    vec3 CameraY = NormalizeOrZero(Cross(CameraZ, CameraX));

    
    float FilmDist = 1.0f;
    float FilmW = 1.0f;
    float FilmH = 1.0f;
    if (WorkOrder->Image->Height > WorkOrder->Image->Width)
    {
        FilmW = FilmH * ((float)WorkOrder->Image->Width / (float)WorkOrder->Image->Height);
    }
    if (WorkOrder->Image->Width > WorkOrder->Image->Height)
    {
        FilmH = FilmW * ((float)WorkOrder->Image->Height / (float)WorkOrder->Image->Width);
    }
    float HalfFilmW = 0.5f * FilmW;
    float HalfFilmH = 0.5f * FilmH;
    vec3 FilmCenter = CameraP - CameraZ * FilmDist;

    float HalfPixW = 0.5f / WorkOrder->Image->Width;
    float HalfPixH = 0.5f / WorkOrder->Image->Height;


    uint32_t RaysPerPixel = 500;
    for (uint32_t Y = WorkOrder->YMin; Y < WorkOrder->OnePastYMax; ++Y)
    {
        uint32_t* Out = GetPixelPointer(WorkOrder->Image, WorkOrder->XMin, Y);
        
        float FilmY = -1.0f + 2.0f * (float)Y / (float)WorkOrder->Image->Height;
        for (uint32_t X = WorkOrder->XMin; X < WorkOrder->OnePastXMax; ++X)
        {
            float FilmX = -1.0f + 2.0f * (float)X / (float)WorkOrder->Image->Width;

            vec3 Color = {};
            float Contrib = 1.0f / (float)RaysPerPixel;
            for (uint32_t RayIndex = 0; RayIndex < RaysPerPixel; ++RayIndex)
            {
                float OffX = FilmX + RandomBilateral() * HalfPixW;
                float OffY = FilmY + RandomBilateral() * HalfPixH;
                vec3 RayOrigin = CameraP;
                vec3 FilmP = FilmCenter + CameraX * OffX * HalfFilmW + CameraY * OffY * HalfFilmH;
                vec3 RayDirection = NormalizeOrZero(FilmP - CameraP);
                Color = Color + (RayCast(Queue, WorkOrder->World, RayOrigin, RayDirection) * Contrib);
            }

            vec4 BMPColor = vec(255.0f * LinearTosRGB(Color.x),
                255.0f * LinearTosRGB(Color.y),
                255.0f * LinearTosRGB(Color.z),
                255.0f);
            uint32_t BMPValue = BGRAPack4x8(BMPColor);

            *Out++ = BMPValue;
        }
    }
    LockedAdd(&Queue->TilesRetired, 1);

    return true;
}

internal DWORD WINAPI WorkerThread(void* lpParameter)
{
    work_queue* Queue = (work_queue*)lpParameter;
    while (RenderTile(Queue)) {};
    return 0;
}

internal void CreateWorkThread(void* Parameter)
{
    DWORD ThreadID;
    HANDLE ThreadHandle = CreateThread(
        0,
        0,
        WorkerThread,
        Parameter,
        0,
        &ThreadID
    );
    CloseHandle(ThreadHandle);
}

int main()
{
    printf("Welcome to Raytracer!\n");

    time_t StartTime = clock();

    material Materials[7] = {};
    Materials[0].EmitColor = vec(0.01f, 0.01f, 0.01f);
    Materials[1].ReflectionColor = vec(0.1f, 0.5f, 0.2f);
    Materials[1].Scatter = 0.001f;
    Materials[2].ReflectionColor = vec(0.235f, 0.737f, 0.803f);
    Materials[3].EmitColor = vec(0.999f, 0.0f, 0.0f);
    Materials[3].Scatter = 0.00001f;
    Materials[4].ReflectionColor = vec(0.090f, 0.95f, 0.074f);
    Materials[4].Scatter = 0.999f;
    Materials[5].ReflectionColor = vec(0.776f, 0.882f, 0.858f);
    Materials[5].Scatter = 0.9999f;
    Materials[6].EmitColor = vec(0.999f, 0.555f, 0.0f);
    Materials[6].Scatter = 0.00001f;


    plane Planes[1] = {};
    Planes[0].N = vec(0, 0, 1);
    Planes[0].d = 0;
    Planes[0].MatIndex = 1;

    sphere Spheres[5] = {};
    Spheres[0].P = vec(0, 0, 0);
    Spheres[0].r = 1.0f;
    Spheres[0].MatIndex = 2;

    Spheres[1].P = vec(2.3f, -2, 1);
    Spheres[1].r = 1.0f;
    Spheres[1].MatIndex = 3;

    Spheres[2].P = vec(1, 6, 2);
    Spheres[2].r = 2.0f;
    Spheres[2].MatIndex = 5;

    Spheres[3].P = vec(-4, 1, 3);
    Spheres[3].r = 1.0f;
    Spheres[3].MatIndex = 4;

    Spheres[4].P = vec(-2, 2, 1);
    Spheres[4].r = 1.0f;
    Spheres[4].MatIndex = 6;

    world World = {};
    World.MaterialCount = ARRAY_COUNT(Materials);
    World.Materials = Materials;
    World.PlaneCount = ARRAY_COUNT(Planes);
    World.Planes = Planes;
    World.SpheresCount = ARRAY_COUNT(Spheres);
    World.Spheres = Spheres;

    uint32_t OutputWidth = 1080;
    uint32_t OutputHeight = 680;
    image Image = CreateImage(OutputWidth, OutputHeight);

    uint32 CoreCount = 4;
    uint32 TileWidth = Image.Width / CoreCount;
    uint32 TileHeight = TileWidth;
    printf("Configurations: %d cores with %dx%d tiles (%d kb/tile).\n",
        CoreCount, TileWidth, TileHeight, (TileWidth * TileHeight * 4) / 1024);

    uint32 TileCountX = (Image.Width + TileWidth - 1) / TileWidth;
    uint32 TileCountY = (Image.Height + TileHeight - 1) / TileHeight;
    uint32 TotalTilesCount = TileCountY * TileCountX;

    work_queue Queue = {};
    Queue.WorkOrders = (work_order*)malloc(TotalTilesCount * sizeof(work_order));
    
    for (uint32 TileY = 0; TileY < TileCountY; ++TileY) 
    {
        uint32 MinY = TileY * TileHeight;
        uint32 OnePastMaxY = MinY + TileHeight;
        if (OnePastMaxY > Image.Height)
        {
            OnePastMaxY = Image.Height;
        }
        for (uint32 TileX = 0; TileX < TileCountX; ++TileX)
        {
            uint32 MinX = TileX * TileWidth;
            uint32 OnePastMaxX = MinX + TileWidth;
            if (OnePastMaxX > Image.Width )
            {
                OnePastMaxX = Image.Width;
            }
            work_order* WorkOrder = Queue.WorkOrders + Queue.WorkOrderCount++;
            assert(Queue.WorkOrderCount <= TotalTilesCount);

            WorkOrder->World = &World;
            WorkOrder->Image = &Image;
            WorkOrder->XMin = MinX;
            WorkOrder->YMin = MinY;
            WorkOrder->OnePastXMax = OnePastMaxX;
            WorkOrder->OnePastYMax = OnePastMaxY;
        }
    }

    // TODO: Memory fencing
#if 1
    for (uint32 CoreIndex = 1; CoreIndex < CoreCount; CoreIndex++)
    {
        CreateWorkThread(&Queue);
    }
#endif

    assert(Queue.WorkOrderCount <= TotalTilesCount);
    while (Queue.TilesRetired < TotalTilesCount)
    {   
        if (RenderTile(&Queue))
        {
            printf("\rRaycasting %d%% ...", 100 * Queue.TilesRetired / TotalTilesCount);
            fflush(stdout);
        }
    }

    time_t EndTime = clock();

    WriteImage(Image, "test.bmp");
    
    printf("\nDone\n");

    time_t TotalTime = EndTime - StartTime;
    printf("\n");
    printf("Time taken: %lld ms\n", TotalTime);
    printf("Total bounces computed: %llu\n", Queue.BouncesComputed);
    printf("Performance: %f ms/bounce", (real64) TotalTime/ (real64) Queue.BouncesComputed);

#ifdef WINDOWS
    system("start test.bmp");
#endif
    
    return 0;
}