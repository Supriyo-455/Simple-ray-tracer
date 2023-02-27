#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <float.h>

#include "ray.h"

typedef int LONG;
typedef unsigned char BYTE;
typedef unsigned int DWORD;
typedef unsigned short WORD;

typedef struct bitmapHeader
{
    WORD bfType;
    DWORD bfSize;
    WORD bfReserved1;
    WORD bfReserved2;
    DWORD bfOffBits;
} __attribute__((packed)) bitmapHeader;

typedef struct bitmapInfo
{
    DWORD biSize;
    LONG biWidth;
    LONG biHeight;
    WORD biPlanes;
    WORD biBitCount;
    DWORD biCompression;
    DWORD biSizeImage;
    LONG biXPelsPerMeter;
    LONG biYPelsPerMeter;
    DWORD biClrUsed;
    DWORD biClrImportant;
} __attribute__((packed)) bitmapInfo;

internal uint32_t GetTotalPixelSize(image Image)
{
    return sizeof(uint32_t) * Image.Height * Image.Width;
}

internal image CreateImage(uint32_t Width, uint32_t Height)
{
    image Image = {};
    Image.Width = Width;
    Image.Height = Height;
    uint32_t OutputPixelSize = GetTotalPixelSize(Image);
    Image.Pixels = (uint32_t *)malloc(OutputPixelSize);
    return Image;
}

internal float RandomUnilateral()
{
    float Result = rand() / (float)RAND_MAX;
    return Result;
}

internal float RandomBilateral()
{
    float Result = 2.0f * RandomUnilateral() - 1.0f;
    return Result;
}

internal vec3 RayCast(world *World, vec3 RayOrigin, vec3 RayDirection)
{
    float MinHitDistance = 0.0001f;
    float Tolerance = 0.00001f;

    vec3 Result = {};
    vec3 Attenuation = vec(1, 1, 1);
    for (uint32_t RayCount = 0; RayCount < 1000; ++RayCount)
    {
        float HitDistance = FLT_MAX;

        vec3 NextNormal = {};
        vec3 NextOrigin = {};
        uint32_t HitMaterialIndex = 0;

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

    return Result;
}

internal void WriteImage(image Image, const char *OutputFileName)
{
    // Bitmap structures to be written to file
    bitmapHeader bfh = {};
    bitmapInfo bih = {};

    uint32_t OutputPixelSize = GetTotalPixelSize(Image);

    // Fill BITMAPFILEHEADER structure
    memcpy((char *)&bfh.bfType, "BM", 2);
    bfh.bfSize = sizeof(bfh) + sizeof(bih) + OutputPixelSize;
    bfh.bfReserved1 = 0;
    bfh.bfReserved2 = 0;
    bfh.bfOffBits = sizeof(bfh) + sizeof(bih);

    // Fill BITMAPINFOHEADER structure
    bih.biSize = sizeof(bih);
    bih.biWidth = Image.Width;
    bih.biHeight = Image.Height;
    bih.biPlanes = 1;
    bih.biBitCount = 32;
    bih.biCompression = 0;      // uncompressed 24-bit RGB
    bih.biSizeImage = 0;        // can be zero for BI_RGB bitmaps
    bih.biXPelsPerMeter = 3780; // 96dpi equivalent
    bih.biYPelsPerMeter = 3780;
    bih.biClrUsed = 0;
    bih.biClrImportant = 0;

    FILE *OutFile = fopen(OutputFileName, "wb");
    if (OutFile)
    {
        fwrite(&bfh, 1, sizeof(bfh), OutFile);
        fwrite(&bih, 1, sizeof(bih), OutFile);
        fwrite(Image.Pixels, OutputPixelSize, 1, OutFile);
        fclose(OutFile);
    }
}

int main()
{
    printf("Welcome to Raytracer!\n");

    material Materials[6] = {};
    Materials[0].EmitColor = vec(0.2f, 0.2f, 0.2f);
    Materials[1].ReflectionColor = vec(0.5f, 0.5f, 0.5f);
    Materials[1].Scatter = 0.00001f;
    Materials[2].ReflectionColor = vec(0.235f, 0.737f, 0.803f);
    Materials[3].EmitColor = vec(0.999f, 0.0f, 0.0f);
    Materials[3].Scatter = 0.00001f;
    Materials[4].ReflectionColor = vec(0.090f, 0.95f, 0.074f);
    Materials[4].Scatter = 0.999f;
    Materials[5].ReflectionColor = vec(0.776f, 0.882f, 0.858f);
    Materials[5].Scatter = 0.78f;

    plane Planes[1] = {};
    Planes[0].N = vec(0, 0, 1);
    Planes[0].d = 0;
    Planes[0].MatIndex = 1;

    sphere Spheres[4] = {};
    Spheres[0].P = vec(0, 0, 0);
    Spheres[0].r = 1.0f;
    Spheres[0].MatIndex = 2;

    Spheres[1].P = vec(2.3f, -2, 1);
    Spheres[1].r = 1.0f;
    Spheres[1].MatIndex = 3;

    Spheres[2].P = vec(1, 6, 2);
    Spheres[2].r = 2.0f;
    Spheres[2].MatIndex = 5;

    Spheres[3].P = vec(-2, 1, 1);
    Spheres[3].r = 1.0f;
    Spheres[3].MatIndex = 4;

    world World = {};
    World.MaterialCount = ARRAY_COUNT(Materials);
    World.Materials = Materials;
    World.PlaneCount = ARRAY_COUNT(Planes);
    World.Planes = Planes;
    World.SpheresCount = ARRAY_COUNT(Spheres);
    World.Spheres = Spheres;

    vec3 CameraP = vec(0, -10, 1);
    vec3 CameraZ = NormalizeOrZero(CameraP);
    vec3 CameraX = NormalizeOrZero(Cross(vec(0, 0, 1), CameraZ));
    vec3 CameraY = NormalizeOrZero(Cross(CameraZ, CameraX));

    uint32_t OutputWidth = 1080;
    uint32_t OutputHeight = 680;
    image Image = CreateImage(OutputWidth, OutputHeight);

    float FilmDist = 1.0f;
    float FilmW = 1.0f;
    float FilmH = 1.0f;
    if (Image.Height > Image.Width)
    {
        FilmW = FilmH * ((float)Image.Width / (float)Image.Height);
    }
    if (Image.Width > Image.Height)
    {
        FilmH = FilmW * ((float)Image.Height / (float)Image.Width);
    }
    float HalfFilmW = 0.5f * FilmW;
    float HalfFilmH = 0.5f * FilmH;
    vec3 FilmCenter = CameraP - CameraZ * FilmDist;

    float HalfPixW = 0.5f / Image.Width;
    float HalfPixH = 0.5f / Image.Height;

    uint32_t RaysPerPixel = 16;
    uint32_t *Out = Image.Pixels;
    for (uint32_t Y = 0; Y < OutputHeight; ++Y)
    {
        float FilmY = -1.0f + 2.0f * (float)Y / (float)Image.Height;
        for (uint32_t X = 0; X < OutputWidth; ++X)
        {
            float FilmX = -1.0f + 2.0f * (float)X / (float)Image.Width;

            vec3 Color = {};
            float Contrib = 1.0f / (float)RaysPerPixel;
            for (uint32_t RayIndex = 0; RayIndex < RaysPerPixel; ++RayIndex)
            {
                float OffX = FilmX + RandomBilateral() * HalfPixW;
                float OffY = FilmY + RandomBilateral() * HalfPixH;
                vec3 RayOrigin = CameraP;
                vec3 FilmP = FilmCenter + CameraX * OffX * HalfFilmW + CameraY * OffY * HalfFilmH;
                vec3 RayDirection = NormalizeOrZero(FilmP - CameraP);
                Color = Color + (RayCast(&World, RayOrigin, RayDirection) * Contrib);
            }

            vec4 BMPColor = vec(255.0f * LinearTosRGB(Color.x),
                                255.0f * LinearTosRGB(Color.y),
                                255.0f * LinearTosRGB(Color.z),
                                255.0f);
            uint32_t BMPValue = BGRAPack4x8(BMPColor);

            *Out++ = BMPValue;
        }
        printf("\rRaycasting %d%% ...", 100 * Y / Image.Height);
        fflush(stdout);
    }

    WriteImage(Image, "test.bmp");

    printf("\nDone\n");
    return 0;
}