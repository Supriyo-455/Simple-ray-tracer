#pragma once


#include "ray_math.h"

struct image
{
    uint32 Width;
    uint32 Height;
    uint32* Pixels;
};

#pragma pack(push, 1)
typedef struct bitmapHeader
{
    unsigned short bfType;
    unsigned int bfSize;
    unsigned short bfReserved1;
    unsigned short bfReserved2;
    unsigned int bfOffBits;
}bitmapHeader;
typedef struct bitmapInfo
{
    unsigned int biSize;
    int biWidth;
    int biHeight;
    unsigned short biPlanes;
    unsigned short biBitCount;
    unsigned int biCompression;
    unsigned int biSizeImage;
    int biXPelsPerMeter;
    int biYPelsPerMeter;
    unsigned int biClrUsed;
    unsigned int biClrImportant;
} bitmapInfo;
#pragma pack(pop)

uint32 GetTotalPixelSize(image Image);
image CreateImage(uint32 Width, uint32 Height);
void WriteImage(image Image, const char* OutputFileName);
uint32* GetPixelPointer(image* Image, uint32 X, uint32 Y);