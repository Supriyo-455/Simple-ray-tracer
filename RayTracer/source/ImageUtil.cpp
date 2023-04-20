#include "ImageUtil.h"

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

void WriteImage(image Image, const char* OutputFileName)
{
    // Bitmap structures to be written to file
    bitmapHeader bfh = {};
    bitmapInfo bih = {};

    uint32_t OutputPixelSize = GetTotalPixelSize(Image);

    // Fill BITMAPFILEHEADER structure
    memcpy((char*)&bfh.bfType, "BM", 2);
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

    FILE* OutFile = fopen(OutputFileName, "wb");
    if (OutFile)
    {
        fwrite(&bfh, 1, sizeof(bfh), OutFile);
        fwrite(&bih, 1, sizeof(bih), OutFile);
        fwrite(Image.Pixels, OutputPixelSize, 1, OutFile);
        fclose(OutFile);
    }
}

uint32* GetPixelPointer(image* Image, uint32 X, uint32 Y)
{
    uint32* Result = Image->Pixels + Y * Image->Width + X;
    return Result;
}

uint32 GetTotalPixelSize(image Image)
{
    return sizeof(uint32) * Image.Height * Image.Width;
}

image CreateImage(uint32 Width, uint32 Height)
{
    image Image = {};
    Image.Width = Width;
    Image.Height = Height;
    uint32_t OutputPixelSize = GetTotalPixelSize(Image);
    Image.Pixels = (uint32_t*)malloc(OutputPixelSize);
    return Image;
}