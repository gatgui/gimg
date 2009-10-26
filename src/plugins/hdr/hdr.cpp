#include <gimg/image.h>
#include <gcore/dmodule.h>
#include <cmath>

struct PixelRGBE {
  unsigned char r;
  unsigned char g;
  unsigned char b;
  unsigned char e;
};

struct PixelRGBF {
  float r;
  float g;
  float b;
};

#define WHITE_EFFICACY 179.0f

static void convertRGBEtoRGBF(const PixelRGBE &in, float exposure, PixelRGBF &out) {
  
  if (in.e != 0) {
    
    //int e = in.e - int(128 + 8);
    //
    //float f = ldexpf(1.0, e) * WHITE_EFFICACY / exposure;
    //
    //out.r = (float)(in.r * f);
    //out.g = (float)(in.g * f);
    //out.b = (float)(in.b * f);
    
    float f = ldexpf(1.0, int(in.e - (128 + 8))) / exposure;
    out.r = (float(in.r) + 0.5f) * f;
    out.g = (float(in.g) + 0.5f) * f;
    out.b = (float(in.b) + 0.5f) * f;
    
  } else {
    
    out.r = out.g = out.b = 0.0f;
  }
}

static void convertRGBFtoRGBE(const PixelRGBF &in, PixelRGBE &out) {
  
  //float r = in.r / WHITE_EFFICACY;
  //float g = in.g / WHITE_EFFICACY;
  //float b = in.b / WHITE_EFFICACY;
  
  float r = in.r;
  float g = in.g;
  float b = in.b;
  
  float v = (r > g ? (r > b ? r : b) : (g > b ? g : b));

  if (v < 1e-32 ) {
    out.r = out.g = out.b = out.e = 0;
    
  } else {
    int e;
    v = frexpf(v, &e) * 256.0f / v;
    
    out.r = (unsigned char)(v * r);
    out.g = (unsigned char)(v * g);
    out.b = (unsigned char)(v * b);
    out.e = (unsigned char)(e + 128);
  }
}

static bool readComponentScanline(FILE *hdrFile,
                                  unsigned char *scanl, unsigned int w) {
  
  unsigned int cur = 0;
  unsigned char p[2];
  
  while (cur < w) {
    
    // read the two first bytes
    fread(p, sizeof(unsigned char), 2, hdrFile);
    
    if (p[0] > 128) {
      // a run
      int run_len = p[0] - 128;
      while (run_len > 0) {
        scanl[cur++] = p[1];
        run_len--;
      }
      
    } else {
      // a non run
      scanl[cur++] = p[1];
      int nonrun_len = p[0] - 1; // -1 because we read the first one already
      if (nonrun_len > 0) {
        fread(scanl+cur, sizeof(unsigned char), nonrun_len, hdrFile);
        cur += nonrun_len;
      }
    }
  }
  
  return (cur == w);
}

static bool writeComponentScanline(FILE *hdrFile,
                                   unsigned char *scanl, unsigned int w) {

  unsigned int cur = 0;
  
  while (cur < w) {
    
    unsigned int start = cur;
    unsigned char prev = scanl[cur++];
    
    if (cur >= w) {
      // we're done, last is a non run
      unsigned char nonrun[2] = {1, prev};
      fwrite(nonrun, sizeof(unsigned char), 2, hdrFile);
    
    } else {
      
      unsigned char next = scanl[cur++];
      
      if (prev == next) {
        
        // we already have two consecutive bytes with the same value
        unsigned char runlen = 2;
        
        // continue until we reach the end of the scanline, a different byte
        // or the maximum size for a run
        while (cur < w && runlen < 127 && scanl[cur] == prev) {
          ++runlen;
          ++cur;
        }
        
        // write the run
        unsigned char run[2] = {((unsigned char)runlen)+128, prev};
        fwrite(run, sizeof(unsigned char), 2, hdrFile);
      
      } else {
        
        prev = next;
        
        // we already have two consecutive bytes with different values
        int nonrunlen = 2;
        
        // continue until we reach the end of the scanline, the maximum size for a non-run
        // or the begining of a run
        while (cur < w && nonrunlen < 127 && scanl[cur] != prev) {
          ++nonrunlen;
          prev = scanl[cur++];
        }
        
        if (cur < w && nonrunlen < 127) {
          // we stopped because we found the begining of a run
          // -> remove last character of the non-run as it will be part of a run
          // -> move cur back to the first character occurence so next loop will
          //    catch up with the run
          nonrunlen -= 1;
          --cur;
        }
        
        // write a non-run
        fwrite(&nonrunlen, sizeof(unsigned char), 1, hdrFile);
        fwrite(scanl+start, sizeof(unsigned char), nonrunlen, hdrFile);
      }
    }
  }
  
  return (cur == w);
}

static void flipVertically(void *pixels, unsigned int width, unsigned int height) {
  if (!pixels) {
    return;
  }
  
  unsigned int n = height / 2;

  PixelRGBF *inout = (PixelRGBF*) pixels;
  
  for (unsigned int x=0; x<width; ++x) {
    
    for (unsigned int y0=0, y1=height-1; y0<n; ++y0, --y1) {
      
      PixelRGBF *sl0 = inout + (y0 * width + x);
      PixelRGBF *sl1 = inout + (y1 * width + x);
      
      PixelRGBF tmp = *sl0;
      *sl0 = *sl1;
      *sl1 = tmp;
    }
  }
}

static void flipHorizontally(void *pixels, unsigned int width, unsigned int height) {
  if (!pixels) {
    return;
  }
  
  unsigned int n = width / 2;
  
  PixelRGBF *inout = (PixelRGBF*) pixels;
  
  for (unsigned int y=0; y<height; ++y) {
    
    PixelRGBF *scanline = inout + (y * width);
    
    for (unsigned int x0=0, x1=width-1; x0<n; ++x0, --x1) {
      
      PixelRGBF tmp = scanline[x0];
      scanline[x0] = scanline[x1];
      scanline[x1] = tmp;
    }
  }
}

static void rotateCW(void *&pixels, unsigned int &width, unsigned int &height) {
  if (!pixels) {
    return;
  }
  
  void *rotated = malloc(width * height * 3 * sizeof(float));
  unsigned int rw = height;
  unsigned int rh = width;
  
  PixelRGBF *in  = (PixelRGBF*) pixels;
  PixelRGBF *out = (PixelRGBF*) rotated;
  PixelRGBF *from, *to;

  for (unsigned int y=0; y<height; ++y) {
    
    for (unsigned int x=0; x<width; ++x) {
      
      from = in + ((width * y) + x);
      to   = out + (x * rw + (rw - 1 - y));

      *to = *from;
    }
  }
  
  free(pixels);
  pixels = rotated;
  width = rw;
  height = rh;
}

static void rotateCCW(void *&pixels, unsigned int &width, unsigned int &height) {
  
  if (!pixels) {
    return;
  }
  
  void *rotated = malloc(width * height * 3 * sizeof(float));
  unsigned int rw = height;
  unsigned int rh = width;
  
  PixelRGBF *in  = (PixelRGBF*) pixels;
  PixelRGBF *out = (PixelRGBF*) rotated;
  PixelRGBF *to, *from;

  for (unsigned int y=0; y<height; ++y) {
    
    for (unsigned int x=0; x<width; ++x) {
      
      from = in + ((y * width) + x);
      to   = out + ((rh - 1 - x) * rw + y);

      *to = *from;
    }
  }
  
  free(pixels);
  pixels = rotated;
  width = rw;
  height = rh;
}


// ---

MODULE_API int numExtensions() {
  return 1;
}

MODULE_API const char* getExtension(int idx) {
  if (idx != 0) {
    return 0;
  } else {
    return "hdr";
  }
}

MODULE_API bool canRead() {
  return true;
}

MODULE_API bool canWrite() {
  return true;
}

MODULE_API gimg::Image* readImage(const char *filepath) {
  
  FILE *hdrFile = fopen(filepath, "rb");
  
  if (!hdrFile) {
    return false;
  }
  
  // header info
  bool isRadiance = false;
  bool readInfo = true;
  bool readHeader = true;
  bool hasFormat = false;
  char headerLine[512];
  bool hflip = false;
  bool vflip = false;
  bool rcw = false;
  bool rccw = false;

  unsigned int width = 0;
  unsigned int height = 0;
  float exposure = 1.0f;  

  // Read header first
  while (!feof(hdrFile) && readHeader) {
    
    fgets(headerLine, 512, hdrFile);
    
    if (!strcmp(headerLine, "#?RADIANCE\n")) {
      isRadiance = true;
      
    } else {
      
      if (!isRadiance) {
        // #?RADIANCE missing
        fclose(hdrFile);
        return 0;
      }
      
      if (readInfo) {
        
        float v0, v1, v2;
        
        if (!strcmp(headerLine, "\n")) {
          // End of header informations
          readInfo = false;
          
        } else if (sscanf(headerLine, "EXPOSURE=%f", &v0) == 1) {
          exposure *= v0;
          
        } else if (!strcmp(headerLine, "FORMAT=32-bit_rle_xyze\n")) {
          // XYZE color format not supported
          fclose(hdrFile);
          return 0;
          
        } else if (!strcmp(headerLine, "FORMAT=32-bit_rle_rgbe\n")) {
          if (hasFormat) {
            // already has a FORMAT header
            fclose(hdrFile);
            return 0;
          }
          hasFormat = true;
          
        } else if (sscanf(headerLine, "COLORCORR=%f %f %f", &v0, &v1, &v2) == 3) {
          // color correction (gamma ?)
        }
        
      } else {
        
        bool validSpec = false;
        char dir0, dir1, axis0, axis1;
        unsigned int dim0, dim1;
        
        if (sscanf(headerLine, "%c%c %u %c%c %u", &dir0, &axis0, &dim0, &dir1, &axis1, &dim1) != 6) {
          // could not parse resolution header
          fclose(hdrFile);
          return 0;
        }
        
        if ((axis0 == 'Y' || axis0 == 'y') &&
            (axis1 == 'X' || axis1 == 'x') &&
            (dir0 == '+' || dir0 == '-') &&
            (dir1 == '+' || dir1 == '-')) {
          
          vflip = (dir0 == '+');
          hflip = (dir1 == '-');
          width = dim1;
          height = dim0;
          validSpec = true;
          
        } else if ((axis0 == 'X' || axis0 == 'x') &&
                   (axis1 == 'Y' || axis1 == 'y') &&
                   (dir0 == '+' || dir0 == '-') &&
                   (dir1 == '+' || dir1 == '-')) {

          rcw = (dir1 == '+');
          rccw = (dir1 == '-');
          vflip = (dir0 != dir1);
          width = dim0;
          height = dim1;
          validSpec = true;
        }
        
        if (!validSpec) {
          // invalid resolution specification
          fclose(hdrFile);
          return 0;
        }
        
        readHeader = false;
      }
    }
  }
  
  // check if we can continue
  if (feof(hdrFile) || readHeader || !hasFormat) {
    fclose(hdrFile);
    return 0;
  }
  
  // Float buffer
  //float *pixels = new float[width * height * 3];
  void *pixels = malloc(width * height * 3 * sizeof(float));
  float *fpixels = (float*) pixels;  

  // Image file scanline
  unsigned char *scanl = new unsigned char[width * 4];
  PixelRGBE &header = *((PixelRGBE*) scanl);
  
  // For each scanline
  for (unsigned int i=0; i<height; ++i) {
    
    // output scanline
    PixelRGBF *outpix = (PixelRGBF*) (fpixels + 3*(i * width));
    
    fread(&header, sizeof(PixelRGBE), 1, hdrFile);
    
    if ((header.r == 2) && (header.g == 2) &&
        ((unsigned int)((header.b << 8) + header.e) == width)) {
      
      // adaptive RLE scanline
      unsigned int roff = 0;
      unsigned int goff = roff + width;
      unsigned int boff = goff + width;
      unsigned int eoff = boff + width;
      
      if (!readComponentScanline(hdrFile, scanl+roff, width) ||
          !readComponentScanline(hdrFile, scanl+goff, width) ||
          !readComponentScanline(hdrFile, scanl+boff, width) ||
          !readComponentScanline(hdrFile, scanl+eoff, width)) {

        fprintf(stdout, "Failed to read component scanline\n");
        fclose(hdrFile);
        delete[] scanl;
        free(pixels);
        return 0;
      }
      
      PixelRGBE inpix;
      for (unsigned int j=0; j<width; ++j) {
        inpix.r = scanl[j+roff];
        inpix.g = scanl[j+goff];
        inpix.b = scanl[j+boff];
        inpix.e = scanl[j+eoff];
        convertRGBEtoRGBF(inpix, exposure, outpix[j]);
      }
      
    } else {
      
      // Uncompressed or RLE scanline (which ?)
      bool uncompressed = true;
      
      // r = g = b = 255 -> runlength of previous pixel, count = exponent

      if (uncompressed) {
        
        size_t rr = fread((void*)(scanl+4), sizeof(unsigned char), 4*(width-1), hdrFile);
        
        if (rr != 4*(width-1)) {
          fclose(hdrFile);
          delete[] scanl;
          free(pixels);
          return 0;
        }
        
        PixelRGBE *inpix = (PixelRGBE*) scanl;
        for (unsigned int j=0; j<width; ++j) {
          convertRGBEtoRGBF(inpix[j], exposure, outpix[j]);
        }
        
      } else {
        
        // Simple RLE encoded
        // if pix with r = g = b = 1 in file -> RLE
        // this means we have to scan them one by one...
        // and that we cannot read the whole scanline at once
      }
    }
    
  }
  
  delete[] scanl;
  
  // do rotations first, then flip
  
  if (rcw) {
    rotateCW(pixels, width, height);
  }
  
  if (rccw) {
    rotateCCW(pixels, width, height);
  }
  
  if (hflip) {
    flipHorizontally(pixels, width, height);
  }
  
  if (vflip) {
    flipVertically(pixels, width, height);
  }
  
  fclose(hdrFile);

  // now create the image
  gimg::PixelDesc desc(gimg::PF_RGB, gimg::PT_FLOAT_32);  
  gimg::Image *img = new gimg::Image(desc, width, height);
  
  void *data = img->getPixels();
  memcpy(data, pixels, width*height*3*sizeof(float));
  free(pixels);

  return img;

}

MODULE_API bool writeImage(gimg::Image *img, const char *filepath) {
  
  if (!img || !filepath) {
    return false;
  }

  unsigned int width = img->getWidth();
  unsigned int height = img->getHeight();

  FILE *hdrFile = fopen(filepath, "wb");
  
  // Write Header
  fprintf(hdrFile, "#?RADIANCE\n");
  fprintf(hdrFile, "FORMAT=32-bit_rle_rgbe\n");
  fprintf(hdrFile, "SOFTWARE=VE R&D HDR Writer\n");
  fprintf(hdrFile, "EXPOSURE=1.0\n");
  //fprintf(hdrFile, "COLORCORR=1.0 1.0 1.0\n"); // is that Gamma ?
  fprintf(hdrFile, "\n");
  fprintf(hdrFile, "-Y %u +X %u\n", height, width);
  
  // Write Pixel Data (RLE)
  float *pixels = (float*) img->getPixels();

  unsigned char *scanl = new unsigned char[width * 4];

  bool compress = false;
  
  if (compress) {
    
    unsigned char *rscanl = scanl;
    unsigned char *gscanl = rscanl + width;
    unsigned char *bscanl = gscanl + width;
    unsigned char *escanl = bscanl + width;
    
    unsigned char header[4];
    
    PixelRGBE outpix;
    
    for (unsigned int y=0; y<height; ++y) {
      
      PixelRGBF *inpix = (PixelRGBF*) (pixels + 3*(y * width));
      
      header[0] = 2;
      header[1] = 2;
      header[2] = (unsigned char)((width >> 8) & 0xFF);
      header[3] = (unsigned char)(width & 0xFF);
      fwrite(header, sizeof(unsigned char), 4, hdrFile);
      
      for (unsigned int x=0; x<width; ++x) {
        convertRGBFtoRGBE(inpix[x], outpix);
        rscanl[x] = outpix.r;
        gscanl[x] = outpix.g;
        bscanl[x] = outpix.b;
        escanl[x] = outpix.e;
      }
      
      if (!writeComponentScanline(hdrFile, rscanl, width) ||
          !writeComponentScanline(hdrFile, gscanl, width) ||
          !writeComponentScanline(hdrFile, bscanl, width) ||
          !writeComponentScanline(hdrFile, escanl, width)) {
        fclose(hdrFile);
        delete[] scanl;
        return false;
      }
    }
    
  } else {
    
    PixelRGBE *outpix = (PixelRGBE*) scanl;
    
    for (unsigned int y=0; y<height; ++y) {
      
      PixelRGBF *inpix = (PixelRGBF*) (pixels + 3*(y * width));
      
      for (unsigned int x=0; x<width; ++x) {
        convertRGBFtoRGBE(inpix[x], outpix[x]);
      }
      fwrite(scanl, sizeof(unsigned char), width*4, hdrFile);
    }
  }
  
  fclose(hdrFile);
  
  delete[] scanl;
  
  return true;
}

