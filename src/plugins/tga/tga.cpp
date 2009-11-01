#include <gimg/image.h>
#include <gcore/dmodule.h>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>

MODULE_API int numExtensions() {
  return 1;
}

MODULE_API const char* getExtension(int idx) {
  if (idx != 0) {
    return 0;
  } else {
    return "tga";
  }
}

MODULE_API bool canRead() {
  return true;
}

MODULE_API bool canWrite() {
  return false;
}

static bool ReadData(std::ifstream &file, char *data, unsigned int sz) {
  if (file.is_open()) {
    unsigned int a = file.tellg();
    a += sz;
    file.read(data, sz);
    if (a == (unsigned int)file.tellg()) {
      return true;
    }
  }
  return false;
}

MODULE_API bool writeImage(gimg::Image *img, const char *filepath) {

#ifdef _DEBUG
  std::cout << "Write TGA to \"" << filepath << "\"" << std::endl;
#endif

  std::ofstream file;

  file.open(filepath, std::ios::binary);

  gimg::PixelDesc desc = img->getPixelDesc();

#ifdef _DEBUG
  std::cout << "Expected image size: "
            << desc.getBytesSizeFor(img->getWidth(), img->getHeight(), 1, 0, 1) << std::endl;
#endif

  if ((desc.getFormat() == gimg::PF_RGB ||
       desc.getFormat() == gimg::PF_RGBA) &&
      desc.getType() == gimg::PT_INT_8) {

#ifdef _DEBUG
    std::cout << "TGA supported image format" << std::endl;
#endif

    unsigned int w = img->getWidth();
    unsigned int h = img->getHeight();
    void *pixels = img->getPixels();

#ifdef _DEBUG
    fprintf(stdout, "  %ux%u %p\n", w, h, pixels);
#endif

    // can write at once, bottom line first in file
    // left to right
    char idlength = 0;
    char colormaptype = 0;
    char imagetype = 2; // RGB

    file.write((const char*)&idlength, 1);
    file.write((const char*)&colormaptype, 1);
    file.write((const char*)&imagetype, 1);

    short colormapstart = 0;
    short colormaplen = 0;
    char colormapbits = 0;

    file.write((const char*)&colormapstart, 2);
    file.write((const char*)&colormaplen, 2);
    file.write((const char*)&colormapbits, 1);

    short xstart = 0;
    short ystart = 0;
    short width = (short) w;
    short height = (short) h;
    char bits = (desc.getFormat() == gimg::PF_RGB ? 24 : 32);
    char flip = 0x20; // bottom row coming first

    file.write((const char*)&xstart, 2);
    file.write((const char*)&ystart, 2);
    file.write((const char*)&width, 2);
    file.write((const char*)&height, 2);    
    file.write((const char*)&bits, 1);
    file.write((const char*)&flip, 1);

    // now write pixel data packed
    size_t ps = (desc.getFormat() == gimg::PF_RGB ? 3 : 4);
    
#ifdef _DEBUG
    size_t pitch = width * ps;
    size_t sz = height * pitch;
    std::cout << "TGA (" << w << "x" << h << ":" << (int)bits << ") pixel data size = " << sz << std::endl;
#endif
    // no, write BGR(A)
    
    const char *pix = (const char*) pixels;
    
    if (ps == 3) {
      for (unsigned int y=0; y<h; ++y) {
        for (unsigned int x=0; x<w; ++x) {
          file.write((const char*)(&pix[2]), 1);
          file.write((const char*)(&pix[1]), 1);
          file.write((const char*)(&pix[0]), 1);
          pix += ps;
        }
      }
    } else {
      for (unsigned int y=0; y<h; ++y) {
        for (unsigned int x=0; x<w; ++x) {
          file.write((const char*)(&pix[2]), 1);
          file.write((const char*)(&pix[1]), 1);
          file.write((const char*)(&pix[0]), 1);
          file.write((const char*)(&pix[3]), 1);
          pix += ps;
        }
      }
    }
    //file.write((const char*)pixels, sz);

    file.close();

    return true;

  } else {
#ifdef _DEBUG
    std::cout << "Save TGA: unsupported image format" << std::endl;
#endif

    return false;
  }
}

MODULE_API gimg::Image* readImage(const char *filepath) {
  std::ifstream file;  
  
  file.open(filepath, std::ios::binary);
  
  gimg::Image *img = 0;

  if (file.is_open()) {
    
    bool rle = false;
    bool not_supported = true;
    
    char idlength;
    char colormaptype;
    char imagetype;

    ReadData(file, &idlength, 1);
    ReadData(file, &colormaptype, 1);
    
    if (colormaptype == 0) {
      
      ReadData(file, &imagetype, 1);
      switch (imagetype) {
      case 2:
        not_supported = false;
        break;
      case 10:
        not_supported = false;
        rle = true;
        break;
      default:
        break;
      }

      if (!not_supported) {

        // skip 5 bytes (colormap specification)
        file.seekg(5, std::ios::cur);

        unsigned short orgx, orgy, w, h;
        unsigned char depth;

        ReadData(file, (char*)&orgx, 2);
        ReadData(file, (char*)&orgy, 2);
        ReadData(file, (char*)&w, 2);
        ReadData(file, (char*)&h, 2);
        ReadData(file, (char*)&depth, 1);

        if (depth==24 || depth==32) {
          // support only RGB or RGBA images
          unsigned char desc;

          ReadData(file, (char*)&desc, 1);

          bool swapCols = ((desc & 0x10) != 0); // last col first in TGA file
          bool swapRows = ((desc & 0x20) == 0); // top rows first in TGA file

          desc = desc & 0x0F;

          if (desc == 0 || desc == 8) {

            // skip identification field [idlength]
            unsigned char bdepth = depth >> 3; // depth in bytes
            unsigned int sz = w * h * bdepth;

#ifdef _DEBUG
            std::cout << "  Plain pixel data size: " << sz << std::endl; 
#endif

            char *pixels = (char*)malloc(sz*sizeof(char));

            file.seekg(idlength, std::ios::cur);

            if (!rle) {
              // Plain data
#ifdef _DEBUG
              std::cout << "  Read plain pixel data" << std::endl;
#endif
              ReadData(file, pixels, sz);

            } else {
#ifdef _DEBUG
              std::cout << "  Read RLE pixel data" << std::endl;
#endif
              // RunLength encoded
              char val;
              char buffer[4];
              char *ptr = pixels;

              unsigned int cpixel = 0;
              unsigned int endpix = w*h - 1;

              while (cpixel < endpix) {
                ReadData(file, &val, 1);
                unsigned char n = (val & 0x7F) + 1;;
                if ((val & 0x80) == 0x80) { //128
                  // a rle packet
                  ReadData(file, buffer, bdepth);
                  for (unsigned int i=0; i<n; ++i) {
                    memcpy(ptr+(i*bdepth), buffer, bdepth);
                  }
                } else {
                  // a raw packet
                  ReadData(file, ptr, n*bdepth);
                }
                cpixel += n;
                ptr += n*bdepth;
              }
            }

            // copy pixels in a new image (TGA pixels BGRA !)
#ifdef _DEBUG
            std::cout << "  Create gimg::Image instance" << std::endl;
#endif

            gimg::PixelDesc desc((bdepth == 4 ? gimg::PF_RGBA : gimg::PF_RGB), gimg::PT_INT_8);

#ifdef _DEBUG
            std::cout << "  Image size: " << w << "x" << h << ":" << (int)depth << ", "
                      << desc.getBytesSizeFor(w, h, 1, 0, 1) << " bytes" << std::endl;
#endif

            img = new gimg::Image(desc, w, h);

            unsigned int pitch = w * bdepth;

            unsigned char *dst = (unsigned char*) img->getPixels();

            if (swapRows) {
              // in TGA file, top rows coming first
              // -> reverse vertical order
#ifdef _DEBUG
              std::cout << "  Swap rows" << std::endl;
#endif
              unsigned int lr = h - 1;
              unsigned int lc = w - 1;

              unsigned char *src = (unsigned char*)(pixels + (lr * pitch));

              if (swapCols) {
#ifdef _DEBUG
                std::cout << "  Swap columns" << std::endl;
#endif
                // in TGA file, row pixels are stored right to left
                // -> reverse horizontal order
                if (bdepth == 4) {
                  for (int j=(int)lr; j>=0; --j) {
                    for (unsigned int k=0; k<w; ++k) {
                      unsigned int od = k * bdepth;
                      unsigned int os = (lc - k) * bdepth;
                      dst[od] = src[os+2];
                      dst[od+1] = src[os+1];
                      dst[od+2] = src[os];
                      dst[od+3] = src[os+3];
                    }
                    src -= pitch;
                    dst += pitch;
                  }
                } else {
                  for (int j=(int)lr; j>=0; --j) {
                    for (unsigned int k=0; k<w; ++k) {
                      unsigned int od = k * bdepth;
                      unsigned int os = (lc - k) * bdepth;
                      dst[od] = src[os+2];
                      dst[od+1] = src[os+1];
                      dst[od+2] = src[os];
                    }
                    src -= pitch;
                    dst += pitch;
                  }
                }
                
              } else {
                if (bdepth == 4) {
                  for (int j=(int)lr; j>=0; --j) {
                    for (unsigned int k=0; k<w; ++k) {
                      unsigned int o = k * bdepth;
                      dst[o] = src[o+2];
                      dst[o+1] = src[o+1];
                      dst[o+2] = src[o];
                      dst[o+3] = src[o+3];
                    }
                    src -= pitch;
                    dst += pitch;
                  }
                } else {
                  for (int j=(int)lr; j>=0; --j) {
                    for (unsigned int k=0; k<w; ++k) {
                      unsigned int o = k * bdepth;
                      dst[o] = src[o+2];
                      dst[o+1] = src[o+1];
                      dst[o+2] = src[o];
                    }
                    src -= pitch;
                    dst += pitch;
                  }
                }
              }

            } else {
              
              unsigned int lc = w - 1;

              char *src = pixels;

              if (swapCols) {
#ifdef _DEBUG
                std::cout << "  Swap columns" << std::endl;
#endif
                // in TGA file, row pixels are stored right to left
                // -> reverse horizontal order
                if (bdepth == 4) {
                  for (unsigned int j=0; j<h; ++j) {
                    for (unsigned int k=0; k<w; ++k) {
                      unsigned int od = k * bdepth;
                      unsigned int os = (lc - k) * bdepth;
                      dst[od] = src[os+2];
                      dst[od+1] = src[os+1];
                      dst[od+2] = src[os];
                      dst[od+3] = src[os+3];
                    }
                    src += pitch;
                    dst += pitch;
                  }
                } else {
                  for (unsigned int j=0; j<h; ++j) {
                    for (unsigned int k=0; k<w; ++k) {
                      unsigned int od = k * bdepth;
                      unsigned int os = (lc - k) * bdepth;
                      dst[od] = src[os+2];
                      dst[od+1] = src[os+1];
                      dst[od+2] = src[os];
                    }
                    src += pitch;
                    dst += pitch;
                  }
                }
                
              } else {
                if (bdepth == 4) {
                  for (unsigned int j=0; j<h; ++j) {
                    for (unsigned int k=0; k<w; ++k) {
                      unsigned int o = k * bdepth;
                      dst[o] = src[o+2];
                      dst[o+1] = src[o+1];
                      dst[o+2] = src[o];
                      dst[o+3] = src[o+3];
                    }
                    src += pitch;
                    dst += pitch;
                  }
                } else {
                  for (unsigned int j=0; j<h; ++j) {
                    for (unsigned int k=0; k<w; ++k) {
                      unsigned int o = k * bdepth;
                      dst[o] = src[o+2];
                      dst[o+1] = src[o+1];
                      dst[o+2] = src[o];
                    }
                    src += pitch;
                    dst += pitch;
                  }
                }
              }
            }

#ifdef _DEBUG
            std::cout << "  Done, free temporary allocated memory" << std::endl;
#endif
            free(pixels);
          }
        }
      }
    }
#ifdef _DEBUG
    std::cout << "  close file" << std::endl;
#endif
    file.close();
  }
  
#ifdef _DEBUG
  if (img) {
    std::cout << "  Write image to \"savetest.tga\"" << std::endl;
    writeImage(img, "savetest.tga");
  }
#endif

  return img;
}

/*
HANDLE ReadTGA(const std::string &fname) {
  std::ifstream file;
  HANDLE dib = NULL;
  file.open(fname.c_str(), std::ios::binary);
  if (file.is_open()) {
    bool rle = false;
    bool not_supported = false;
    char idlength;
    char colormaptype;
    char imagetype;
    ReadData(file, &idlength, 1);
    ReadData(file, &colormaptype, 1);
    if (colormaptype == 0) {
      ReadData(file, &imagetype, 1);
      switch (imagetype) {
        case 2:
          not_supported = false;
          break;
        case 10:
          rle = true;
          not_supported = false;
          break;
        default:
          break;
      }
      if (!not_supported) {
        // skip 5 bytes (colormap specification)
        file.seekg(5, std::ios::cur);
        unsigned short orgx, orgy, w, h;
        unsigned char depth;
        ReadData(file, (char*)&orgx, 2);
        ReadData(file, (char*)&orgy, 2);
        ReadData(file, (char*)&w, 2);
        ReadData(file, (char*)&h, 2);
        ReadData(file, (char*)&depth, 1);
        if (depth==24 || depth==32) {
          // only RGB or RGBA images
          unsigned char desc;
          ReadData(file, (char*)&desc, 1);
#ifdef HANDLE_PIX_ORG
          bool swapCols = ((desc & 0x10) != 0); // last col first 
          bool swapRows = ((desc & 0x20) != 0); // last row first
#endif // HANDLE_PIX_ORG
          desc = desc & 0x0F;
          if (desc == 0 || desc == 8) {
            // skip identification field [idlength]
            unsigned char bdepth = depth >> 3; // depth in bytes
            unsigned int sz = w * h * bdepth;
            char *pixels = (char*)malloc(sz*sizeof(char));
            file.seekg(idlength, std::ios::cur);
            if (!rle) {
              ReadData(file, pixels, sz);
            } else {
              // RunLength encoded
              char val;
              char buffer[4];
              char *ptr = pixels;
              unsigned int cpixel = 0;
              unsigned int endpix = w*h - 1;
              while (cpixel < endpix) {
                ReadData(file, &val, 1);
                unsigned char n = (val & 0x7F) + 1;;
                if ((val & 0x80) == 0x80) { //128
                  // a rle packet
                  ReadData(file, buffer, bdepth);
                  for (unsigned int i=0; i<n; ++i) {
                    memcpy(ptr+(i*bdepth), buffer, bdepth);
                  }
                } else {
                  // a raw packet
                  ReadData(file, ptr, n*bdepth);
                }
                cpixel += n;
                ptr += n*bdepth;
              }
            }
            // Keep BGRA format
            // Create DIB
            unsigned int dib_line_sz = ((((w * depth) + 31) & ~31) >> 3);
            unsigned int bmpImageSize = dib_line_sz * h;
            dib = GlobalAlloc(GMEM_MOVEABLE, sizeof(BITMAPINFOHEADER)+bmpImageSize);
            LPBITMAPINFOHEADER lpbi = (LPBITMAPINFOHEADER)GlobalLock(dib);
            memset(lpbi, 0, sizeof(BITMAPINFOHEADER));
            lpbi->biSize = sizeof(BITMAPINFOHEADER);
            lpbi->biSizeImage = bmpImageSize;
            lpbi->biBitCount = depth;
            lpbi->biPlanes = 1;
            lpbi->biWidth = w;
            lpbi->biHeight = h;
            unsigned int src_line_sz = w * bdepth;
            char *dst = (char*)lpbi + sizeof(BITMAPINFOHEADER);
#ifdef HANDLE_PIX_ORG
            if (swapRows) {
#ifdef _DEBUG
              std::cout << "Swap rows" << std::endl;
#endif
              // in TGA file, top rows coming first
              // need to reverse the order for BMP
              unsigned int lr = h - 1;
              unsigned int lc = w - 1;
              //unsigned int lc = src_line_sz - 1;
              char *src = pixels + (lr * src_line_sz);
              if (swapCols) {
#ifdef _DEBUG
                std::cout << "Swap cols" << std::endl;
#endif
                // in TGA file, rows are store from right to left
                // need to reverse the order for BMP
                for (int j=(int)lr; j>=0; --j) {
                  if (bdepth == 3) {
                    for (unsigned int k=0; k<w; ++k) {
                      unsigned int od = k*3;
                      unsigned int os = (lc-k)*3;
                      dst[od]   = src[os];
                      dst[od+1] = src[os+1];
                      dst[od+2] = src[os+2];
                    }
                  } else {
                    for (unsigned int k=0; k<w; ++k) {
                      unsigned int od = k*4;
                      unsigned int os = (lc-k)*4;
                      dst[od]   = src[os];
                      dst[od+1] = src[os+1];
                      dst[od+2] = src[os+2];
                      dst[od+4] = src[os+4];
                    }
                  }
                  //for (unsigned int k=0; k<src_line_sz; ++k) {
                  //  dst[k] = src[lc-k];
                  //}
                  src -= src_line_sz;
                  dst += dib_line_sz;
                }
              } else {
                for (int j=(int)lr; j>=0; --j) {
                  memcpy(dst, src, src_line_sz);
                  src -= src_line_sz;
                  dst += dib_line_sz;
                }
              }
            } else {
              //unsigned int lc = src_line_sz - 1;
              unsigned int lc = w - 1;
              char *src = pixels;
              if (swapCols) {
#ifdef _DEBUG
                std::cout << "Swap cols" << std::endl;
#endif
                // in TGA file, rows are store from right to left
                // need to reverse the order for BMP
                for (unsigned int j=0; j<h; ++j) {
                  if (bdepth == 3) {
                    for (unsigned int k=0; k<w; ++k) {
                      unsigned int od = k*3;
                      unsigned int os = (lc-k)*3;
                      dst[od]   = src[os];
                      dst[od+1] = src[os+1];
                      dst[od+2] = src[os+2];
                    }
                  } else {
                    for (unsigned int k=0; k<w; ++k) {
                      unsigned int od = k*4;
                      unsigned int os = (lc-k)*4;
                      dst[od]   = src[os];
                      dst[od+1] = src[os+1];
                      dst[od+2] = src[os+2];
                      dst[od+4] = src[os+4];
                    }
                  }
                  //for (unsigned int k=0; k<src_line_sz; ++k) {
                  //  dst[k] = src[lc-k];
                  //}
                  src += src_line_sz;
                  dst += dib_line_sz;
                }
              } else {
                for (unsigned int j=0; j<h; ++j) {
                  memcpy(dst, src, src_line_sz);
                  src += src_line_sz;
                  dst += dib_line_sz;
                }
              }
            }
#else // HANDLE_PIX_ORG
            char *src = pixels;
            for (unsigned int j=0; j<h; ++j) {
              memcpy(dst, src, src_line_sz);
              src += src_line_sz;
              dst += dib_line_sz;
            }
#endif // HANDLE_PIX_ORG
            GlobalUnlock(dib);
            free(pixels);
          }
        }
      }
    }
    file.close();
  }
  return dib;
}
*/
