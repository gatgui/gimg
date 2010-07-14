#include <gimg/image.h>
#include <gcore/dmodule.h>
#include <cmath>
#include <string>
#include <cstdio>
#include <cstdlib>

typedef long           LONG;
typedef unsigned long  DWORD;
typedef int            BOOL;
typedef unsigned char  BYTE;
typedef unsigned short WORD;
typedef float          FLOAT;

#ifndef _WIN32

typedef struct tagRGBQUAD {
  BYTE  rgbBlue;
  BYTE  rgbGreen;
  BYTE  rgbRed;
  BYTE  rgbReserved;
} RGBQUAD;

typedef struct tagBITMAPFILEHEADER { 
  WORD   bfType;
  DWORD  bfSize;
  WORD   bfReserved1;
  WORD   bfReserved2;
  DWORD  bfOffBits;
} BITMAPFILEHEADER;

typedef struct tagBITMAPCOREHEADER {
  DWORD  bcSize; 
  WORD   bcWidth; 
  WORD   bcHeight; 
  WORD   bcPlanes; 
  WORD   bcBitCount; 
} BITMAPCOREHEADER;

typedef struct tagBITMAPINFOHEADER{
  DWORD  biSize; 
  LONG   biWidth; 
  LONG   biHeight; 
  WORD   biPlanes; 
  WORD   biBitCount; 
  DWORD  biCompression; 
  DWORD  biSizeImage; 
  LONG   biXPelsPerMeter; 
  LONG   biYPelsPerMeter; 
  DWORD  biClrUsed; 
  DWORD  biClrImportant; 
} BITMAPINFOHEADER;

#define BI_RGB        0
#define BI_RLE8       1
#define BI_RLE4       2
#define BI_BITFIELDS  3
#define BI_JPEG       4
#define BI_PNG        5

#endif

#define BF_SIZE sizeof(BITMAPFILEHEADER)
#define BI_SIZE sizeof(BITMAPINFOHEADER)
#define C_SIZE  sizeof(RGBQUAD)
#define BM      0x4d42 // 'BM'

// ---

MODULE_API int numExtensions() {
  return 1;
}

MODULE_API const char* getExtension(int idx) {
  if (idx != 0) {
    return 0;
  } else {
    return "bmp";
  }
}

MODULE_API bool canRead() {
  return true;
}

MODULE_API bool canWrite() {
  return false;
}

#define CHECK_READ(dst, type, file, msg) \
  if (fread((void*)dst, sizeof(type), 1, file) != 1) {\
    std::cout << "Could not read data from BMP file (" << msg << ")" << std::endl;\
    return 0;\
  }

MODULE_API gimg::Image* readImage(const char *filepath) {
  // dib is not in the format we want 
  // PF_RGB | PF_RGBA (24 or 32 bits)
  // -> NOT BGR beware !
  
  gimg::Image *bmp = 0;
  
  BITMAPFILEHEADER bf;
  BITMAPINFOHEADER bi;
  
  memset(&bf, 0, BF_SIZE);
  memset(&bi, 0, BI_SIZE);
  
  FILE *bitmap = fopen(filepath, "rb");
  
  if (bitmap) {
    
    // Read file header and check
    CHECK_READ(&(bf.bfType), WORD, bitmap, "type");
    if (bf.bfType != BM) {
      std::cerr << "Un-supported BMP type" << std::endl;
      return 0;
    }
    CHECK_READ(&(bf.bfSize), DWORD, bitmap, "header size");
    CHECK_READ(&(bf.bfReserved1), WORD, bitmap, "reserved1");
    CHECK_READ(&(bf.bfReserved2), WORD, bitmap, "reserved2");
    CHECK_READ(&(bf.bfOffBits), DWORD, bitmap, "data offset");
    
    // Read info header and check
    CHECK_READ(&(bi.biSize), DWORD, bitmap, "info size");
    if (bi.biSize <= sizeof(BITMAPCOREHEADER)) {
      std::cerr << "BMP file format not supported" << std::endl;
    }
    CHECK_READ(&(bi.biWidth), LONG, bitmap, "width");
    CHECK_READ(&(bi.biHeight), LONG, bitmap, "height");
    CHECK_READ(&(bi.biPlanes), WORD, bitmap, "planes");
    CHECK_READ(&(bi.biBitCount), WORD, bitmap, "bpp");
    if (bi.biBitCount < 24) {
      std::cerr << "BMP file format not supported, expected 24/32 bits color type" << std::endl;
      return 0;
    }
#ifdef _DEBUG
    std::cout << "BMP: " << bi.biWidth << "x" << bi.biHeight << ":" << bi.biBitCount << std::endl;
#endif
    CHECK_READ(&(bi.biCompression), DWORD, bitmap, "compression");
    if (bi.biCompression > BI_RGB) {
      std::cerr << "BMP file format not supported, expected non-compressed data" << std::endl;
      return 0;
    }
    CHECK_READ(&(bi.biSizeImage), DWORD, bitmap, "data size");
    CHECK_READ(&(bi.biXPelsPerMeter), LONG, bitmap, "x units");
    CHECK_READ(&(bi.biYPelsPerMeter), LONG, bitmap, "y units");
    CHECK_READ(&(bi.biClrUsed), DWORD, bitmap, "used color count");
    if (bi.biClrUsed > 0) {
      std::cerr << "Indexed color BMP files not suppored" << std::endl;
      return 0;
    }
    CHECK_READ(&(bi.biClrImportant), DWORD, bitmap, "important color count");
    
    // beware bi.biHeight could be < 0 -> origin top left
    bool hflip = false;
    if (bi.biHeight < 0) {
      hflip = true;
      bi.biHeight = -bi.biHeight;
    }
    
    // pitch is not just BytesPerPixel * W
    // in bitmap file, scanline byte size is adjusted to nearest multiple of 4 size
    // the " >> 3" is to convert bit size to byte size
    // +31 is to make sure we will not reduce the width when rounding up
    // ~31 clear the last 5 bits (summing to 31) in order to make size a multiple of 32
    
    unsigned long pitch = (((bi.biWidth * bi.biBitCount) + 31) & ~31) >> 3;
    
    unsigned long imgsz = bi.biSizeImage;
    
#ifdef _DEBUG
    std::cout << "gimg::Image picth: " << (bi.biWidth * 3) << std::endl;
    std::cout << "BMP image picth: " << pitch << std::endl;
#endif
    
    if (imgsz == 0) {
      imgsz = pitch * bi.biHeight;
    }
    
    void *dib = malloc(imgsz);
    if (!dib) {
      std::cerr << "Could not allocate memory to read BMP file data" << std::endl;
      return 0;
    }
    
    if (bf.bfOffBits != 0) {
#ifdef _DEBUG
      std::cout << "BMP file offset set: " << bf.bfOffBits << std::endl;
#endif
      fseek(bitmap, bf.bfOffBits, SEEK_SET);
    }
    
    if (fread(dib, 1, imgsz, bitmap) != imgsz) {
      free(dib);
      std::cerr << "Could not read BMP file data" << std::endl;
      return 0;
    }
    
    // now copy scanlines and we are done, use RGBQUAD because bitmap are store in BGR format
    // there absolutely no alpha in bitmap (even on 32 mode ?)
    // should be pretty easy from here
    
    gimg::PixelDesc desc(gimg::PF_RGB, gimg::PT_INT_8);
    
    bmp = new gimg::Image(desc, bi.biWidth, bi.biHeight);
    
    size_t srcPixSize = bi.biBitCount >> 3;
    
    void *dstPixels = bmp->getPixels();
    size_t dstPitch = bi.biWidth * desc.getBytesPerPixel();
    
#ifdef _DEBUG
    std::cout << "gimg::Image pitch (from PixelDesc): " << dstPitch << std::endl;
#endif
    
    for (LONG y=0; y<bi.biHeight; ++y) {
    
      // top rows coming first
      unsigned char* dibScanline = (unsigned char*)dib + (((bi.biHeight - 1) - y) * pitch);
      
      unsigned char* bmpScanline = ((unsigned char*)dstPixels) + (y * dstPitch);
      
      for (LONG x=0; x<bi.biWidth; ++x) {
        
        bmpScanline[3*x+0] = dibScanline[2];
        bmpScanline[3*x+1] = dibScanline[1];
        bmpScanline[3*x+2] = dibScanline[0];
        
        dibScanline += srcPixSize;
      }
    }
    
    free(dib);
    
    fclose(bitmap);
  }
  
#ifdef _DEBUG
  std::cout << "Image read successfully !" << std::endl;
#endif
  
  return bmp;
}

MODULE_API bool writeImage(gimg::Image *, const char*) {
  return false;
}
