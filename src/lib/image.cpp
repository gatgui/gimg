/*

Copyright (C) 2009  Gaetan Guidet

This file is part of gimg.

gimg is free software; you can redistribute it and/or modify it
under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or (at
your option) any later version.

gimg is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
USA.

*/

#ifdef _MSC_VER
# ifndef _USE_MATH_DEFINES
#   define _USE_MATH_DEFINES
# endif
#endif
#include <gimg/image.h>
#include <limits>
#include <cmath>
#include <cassert>

namespace gimg {
  
  gcore::List<Image::Plugin*> Image::msPlugins;
  Image::PluginMap Image::msReaders;
  Image::PluginMap Image::msWriters;

  static void mipmap_int8(void *p0, void *p1, void *p2, void *p3, int n, void *to) {
    unsigned char *c0 = (unsigned char *) p0;
    unsigned char *c1 = (unsigned char *) p1;
    unsigned char *c2 = (unsigned char *) p2;
    unsigned char *c3 = (unsigned char *) p3;
    unsigned char *r  = (unsigned char *) to;
    for (int i=0; i<n; ++i) {
      unsigned short tmp0 = ((unsigned short)*c0 + (unsigned short)*c1) / 2;
      unsigned short tmp1 = ((unsigned short)*c2 + (unsigned short)*c3) / 2;
      unsigned short res  = (tmp0 + tmp1) / 2;
      *r = (res > 255) ? 255 : (unsigned char)res;
      ++c0; ++c1; ++c2; ++c3; ++r;
    }
  }

  static void mipmap_int16(void *p0, void *p1, void *p2, void *p3, int n, void *to) {
    unsigned short *c0 = (unsigned short *) p0;
    unsigned short *c1 = (unsigned short *) p1;
    unsigned short *c2 = (unsigned short *) p2;
    unsigned short *c3 = (unsigned short *) p3;
    unsigned short *r  = (unsigned short *) to;
    for (int i=0; i<n; ++i) {
      unsigned long tmp0 = ((unsigned long)*c0 + (unsigned long)*c1) / 2;
      unsigned long tmp1 = ((unsigned long)*c2 + (unsigned long)*c3) / 2;
      unsigned long res  = (tmp0 + tmp1) / 2;
      *r = (res > 65535) ? 65535 : (unsigned short)res;
      ++c0; ++c1; ++c2; ++c3; ++r;
    }
  }

  static void mipmap_int32(void *p0, void *p1, void *p2, void *p3, int n, void *to) {
    unsigned long *c0 = (unsigned long *) p0;
    unsigned long *c1 = (unsigned long *) p1;
    unsigned long *c2 = (unsigned long *) p2;
    unsigned long *c3 = (unsigned long *) p3;
    unsigned long *r  = (unsigned long *) to;
    for (int i=0; i<n; ++i) {
      unsigned long tmp0 = (*c0 + *c1) / 2;
      unsigned long tmp1 = (*c2 + *c3) / 2;
      *r  = (tmp0 + tmp1) / 2;
      ++c0; ++c1; ++c2; ++c3; ++r;
    }
  }

  static void mipmap_float(void *p0, void *p1, void *p2, void *p3, int n, void *to) {
    float *c0 = (float *) p0;
    float *c1 = (float *) p1;
    float *c2 = (float *) p2;
    float *c3 = (float *) p3;
    float *r  = (float *) to;
    for (int i=0; i<n; ++i) {
      float tmp0 = (*c0 + *c1) / 2.0f;
      float tmp1 = (*c2 + *c3) / 2.0f;
      *r  = (tmp0 + tmp1) / 2.0f;
      ++c0; ++c1; ++c2; ++c3; ++r;
    }
  }
  
  
  // Resizing image

  class Filter {
    public:
      Filter(double width)
        :mWidth(width) {
      }
      virtual ~Filter() {
      }
      virtual double weight(double pos) const = 0;
      inline double width() const {
        return mWidth;
      }
    protected:
      double mWidth;
  };

  class BoxFilter : public Filter {
    public:
      BoxFilter() : Filter(0.5) {
      }
      virtual ~BoxFilter() {
      }
      virtual double weight(double pos) const {
        pos = fabs(pos);
        if (pos < width()) {
          return 1.0;
        }
        return 0.0;
      }
  };

  class LinearFilter : public Filter {
    public:
      LinearFilter() : Filter(1.0) {
      }
      virtual ~LinearFilter() {
      }
      virtual double weight(double pos) const {
        pos = fabs(pos);
        if (pos < width()) {
          return (width() - pos);
        }
        return 0.0;
      }
  };

  class CubicFilter : public Filter {
    public:
      CubicFilter() : Filter(2.0) {
        double b = 1.0 / 3.0; // blurring
        double c = 1.0 / 3.0; // ringing
        double s = 1.0 / 6.0;
        p0 = (6 - 2*b) * s;
        p2 = (-18 + 12*b + 6*c) * s;
        p3 = (12 - 9*b - 6*c) * s;
        q0 = (8*b + 24*c) * s;
        q1 = (-12*b - 48*c) * s;
        q2 = (6*b + 30*c) * s;
        q3 = (-b - 6*c) * s;
      }
      virtual ~CubicFilter() {
      }
      virtual double weight(double pos) const {
        pos = fabs(pos);
        if (pos < 1) {
          return (p0 + pos*pos*(p2 + pos*p3));
        }
        if (pos < 2) {
          return (q0 + pos*(q1 + pos*(q2 + pos*q3)));
        }
        return 0.0;
      }
    protected:
      double p0, p2, p3;
      double q0, q1, q2, q3;
  };

  class LanczosFilter : public Filter {
    public:
      LanczosFilter() : Filter(3.0) {
      }
      virtual ~LanczosFilter() {
      }
      virtual double weight(double pos) const {
        if (pos >= -width() && pos <= width()) {
          if (fabs(pos) < 1e-6) {
            return 1.0;
          } else {
            // = sinc(pos) * sinc(pos/width())
            pos *= M_PI;
            return (width() * sin(pos) * sin(pos/width()) / (pos * pos));
          }
        }
        return 0.0;
      }
  };

  class FilterWeights {
    protected:
      struct PixelWeights {
        std::vector<double> weights;
        unsigned int start;
        unsigned int length;
      };
    public:
      FilterWeights() {
      }
      FilterWeights(Filter *filter, unsigned int srcSize, unsigned int dstSize) {
        initialize(filter, srcSize, dstSize);
      }
      ~FilterWeights() {
        mWeightsTable.clear();
      }
      void initialize(Filter *filter, unsigned int srcSize, unsigned dstSize) {
        assert(filter != 0);
        
        double scale = double(dstSize) / double(srcSize);
        double width = filter->width();
        double fscale = 1.0;
        
        if (scale < 1.0) {
          // when minimizing, increase filter width
          width /= scale;
          fscale = scale;
        }
        
        double iscale = 1.0 / scale;
        
        unsigned int windowSize = 2 * ((unsigned int) ceil(width)) + 1;
        
        unsigned int i, j;
        
        // init weight table
        mWeightsTable.resize(dstSize);
        for (i=0; i<dstSize; ++i) {
          mWeightsTable[i].weights.resize(windowSize, 0.0);
          mWeightsTable[i].start = 0;
          mWeightsTable[i].length= 0;
        }
        
        // initialize weights for each pixel
        
        for (i=0; i<dstSize; ++i) {
          
          PixelWeights &pw = mWeightsTable[i];
          
          double srcX = (double(i) + 0.5) * iscale; // - 0.5 ?
          
          unsigned int start = (unsigned int) maxval(0.0, floor(srcX-width));
          unsigned int stop = (unsigned int) minval(ceil(srcX+width), double(srcSize)-1);
          unsigned int len = minval(windowSize, stop-start);
          
          pw.start = start;
          pw.length = len;
          
          double totalWeights = 0.0;
          
          for (j=0; j<len; ++j) {
            double filterPos = double(start+j) + 0.5 - srcX;
            
            pw.weights[j] = filter->weight(filterPos * fscale);
            totalWeights += pw.weights[j];
          }
          
          if (totalWeights > 0.0 && totalWeights != 1.0) {
            // normalize
            totalWeights = 1.0 / totalWeights;
            for (j=0; j<len; ++j) {
              pw.weights[j] *= totalWeights;
            }
            // simplify
            j = len - 1;
            while (fabs(pw.weights[j]) < 1e-9) {
              --j;
              pw.length--;
              if (pw.length == 0) {
                break;
              }
            }
          }
        }
      }
      inline unsigned int firstPixel(unsigned int dstPos) const {
        return mWeightsTable[dstPos].start;
      }
      inline unsigned int numPixels(unsigned int dstPos) const {
        return mWeightsTable[dstPos].length;
      }
      inline double pixelWeight(unsigned int dstPos, unsigned int idx) const {
        return mWeightsTable[dstPos].weights[idx];
      }
    protected:
      template <typename T> inline T maxval(T v0, T v1) {
        return (v0 > v1 ? v0 : v1);
      }
      template <typename T> inline T minval(T v0, T v1) {
        return (v0 < v1 ? v0 : v1);
      }
    protected:
      std::vector<PixelWeights> mWeightsTable;
  };

  typedef void (*PixelInitFunc)(unsigned int, void *dst);
  typedef void (*PixelAccumFunc)(unsigned int, void *dst, double weight, void *src);
  
  static void* scaleVertical(void *src, unsigned int width, unsigned int height,
                             unsigned pixChannels, unsigned int pixSize,
                             Filter *filter, unsigned int newHeight,
                             PixelInitFunc pixInit, PixelAccumFunc pixAccum) {
    
    FilterWeights weights(filter, height, newHeight);
    
    unsigned int rowSize = width * pixSize;
    
    unsigned char *srcImg = (unsigned char*) src;
    unsigned char *dstImg = (unsigned char*) malloc(newHeight * rowSize);
    
    for (unsigned int i=0; i<width; ++i) {
      
      unsigned char *srcCol = srcImg + (i * pixSize);
      unsigned char *dstCol = dstImg + (i * pixSize);
      
      for (unsigned int j=0; j<newHeight; ++j) {
        
        unsigned char *dstPix = dstCol + (j * rowSize);
        
        pixInit(pixChannels, dstPix);
        
        unsigned int s = weights.firstPixel(j);
        unsigned int n = weights.numPixels(j);
        
        for (unsigned int k=0; k<n; ++k) {
          
          double weight = weights.pixelWeight(j, k);
          
          unsigned char *srcPix = srcCol + ((s + k) * rowSize);
          
          pixAccum(pixChannels, dstPix, weight, srcPix);
        }
      }
    }
    
    return dstImg;
  }
  
  static void* scaleHorizontal(void *src, unsigned int width, unsigned int height,
                               unsigned pixChannels, unsigned int pixSize,
                               Filter *filter, unsigned int newWidth,
                               PixelInitFunc pixInit, PixelAccumFunc pixAccum) {
    
    FilterWeights weights(filter, width, newWidth);
    
    unsigned int srcRowSize = width * pixSize;
    unsigned int dstRowSize = newWidth * pixSize;
    
    unsigned char *srcImg = (unsigned char*) src;
    unsigned char *dstImg = (unsigned char*) malloc(height * dstRowSize);
    
    for (unsigned int i=0; i<height; ++i) {
      
      unsigned char *srcRow = srcImg + (i * srcRowSize);
      unsigned char *dstRow = dstImg + (i * dstRowSize);
      
      for (unsigned int j=0; j<newWidth; ++j) {
        
        unsigned char *dstPix = dstRow + (j * pixSize);
        
        pixInit(pixChannels, dstPix);
        
        unsigned int s = weights.firstPixel(j);
        unsigned int n = weights.numPixels(j);
        
        for (unsigned int k=0; k<n; ++k) {
          
          double weight = weights.pixelWeight(j, k);
          
          unsigned char *srcPix = srcRow + ((s + k) * pixSize);
          
          pixAccum(pixChannels, dstPix, weight, srcPix);
        }
      }
    }
    
    return dstImg;
  }
  
  template <typename T>
  static void pixInitT(unsigned int nChan, void *pix) {
    T *pixt = (T*)pix;
    for (unsigned int i=0; i<nChan; ++i) {
      pixt[i] = T(0);
    }
  }
  
  template <typename T>
  static void pixAccumTClamped(unsigned int nChan, void *dst, double w, void *src) {
    
    static double sToT = double((std::numeric_limits<T>::max)());
    static double sToD = 1.0 / sToT;
    
    T *tsrc = (T *)src;
    T *tdst = (T *)dst;
    
    for (unsigned int i=0; i<nChan; ++i) {
      double v = sToD*tdst[i] + w * (sToD*tsrc[i]);
      v = (v > 1.0 ? 1.0 : (v < 0.0 ? 0.0 : v));
      tdst[i] = T(floor(v * sToT));
    }
  }
  
  template <typename T>
  static void pixAccumT(unsigned int nChan, void *dst, double w, void *src) {
    
    T *tsrc = (T *)src;
    T *tdst = (T *)dst;
    
    for (unsigned int i=0; i<nChan; ++i) {
      double v = double(tdst[i]) + w * double(tsrc[i]);
      tdst[i] = T(v);
    }
  }
  
  static bool EnumPlugins(const gcore::Path &path) {
    if (path.isFile()) {
      
      if (path.checkExtension("ipl")) {
        
        Image::Plugin *plg = new Image::Plugin(path.fullname());
        if (plg->_opened()) {
          if (!Image::RegisterPlugin(plg)) {
            delete plg;
          } else {
            std::cout << "Loaded image plugin: \"" << path.basename() << "\"" << std::endl;
          }
        } else {
          delete plg;
        }
      }
    }
    return true;
  }

  // ---

  bool Image::RegisterPlugin(Image::Plugin *plugin) {
    
    bool added = false;
    
    for (int i=0; i<plugin->numExtensions(); ++i) {
      
      const char *ext = plugin->getExtension(i);
      
      if (plugin->canRead() && msReaders.find(ext) == msReaders.end()) {
        added = true;
        msReaders[ext] = plugin;
      }
      
      if (plugin->canWrite() && msWriters.find(ext) == msWriters.end()) {
        added = true;
        msWriters[ext] = plugin;
      }
    }
    
    if (added) {
      msPlugins.push_back(plugin);
    }
    
    return added;
  }

  bool Image::UnregisterPlugin(Image::Plugin *plugin) {
    std::vector<Image::Plugin*>::iterator pit = 
      std::find(msPlugins.begin(), msPlugins.end(), plugin);
    
    if (pit == msPlugins.end()) {
      return false;
    }
    
    Image::PluginMap::iterator it;
    
    if (plugin->canRead()) {
      it = msReaders.begin();
      while (it != msReaders.end()) {
        if (it->second == plugin) {
          msReaders.erase(it);
          it = msReaders.begin();
        } else {
          ++it;
        }
      }
    }
    
    if (plugin->canWrite()) {
      it = msWriters.begin();
      while (it != msWriters.end()) {
        if (it->second == plugin) {
          msWriters.erase(it);
          it = msWriters.begin();
        } else {
          ++it;
        }
      }
    }
    
    delete *pit;
    msPlugins.erase(pit);
    
    return true;
  }
  
  void Image::LoadPlugins(const gcore::Path &path) {
    gcore::Path::EachFunc callback;
    gcore::Bind(EnumPlugins, callback);
    path.each(callback);
  }
  
  void Image::UnloadPlugins() {
    while (msPlugins.size() > 0) {
      UnregisterPlugin(msPlugins[0]);
    }
    msPlugins.clear();
  }
  
  Image* Image::Read(const gcore::Path &filepath, int numMips) {
    
    gcore::String ext = filepath.getExtension();
    
    PluginMap::iterator it = msReaders.find(ext.c_str());
    
    if (it != msReaders.end()) {
      Image *img = it->second->readImage(filepath.fullname().c_str());
      if (img) {
        if (img->getNumMipmaps() <= 0 && numMips > 0) {
          img->buildMipmaps(numMips);
        }
        return img;
      }
    }
    
    return 0;
  }
  
  bool Image::Write(Image *img, const gcore::Path &filepath) {
    gcore::String ext = filepath.getExtension();
    
    PluginMap::iterator it = msWriters.find(ext.c_str());
    
    if (it != msWriters.end()) {
      return it->second->writeImage(img, filepath.fullname().c_str());
    }
    
    return false;
  }


  // ---

  Image::Image(PixelDesc desc, int w, int h, int d, int numMipmaps)
    :mMaxWidth(w), mMaxHeight(h), mMaxDepth(d),
     mNumMipmaps(numMipmaps), mDesc(desc) {

    // adjust mipmap count if needed
    int maxMipmaps = desc.getMaxMipmaps(w, h, d);
    
    if (mNumMipmaps < 0 || mNumMipmaps > maxMipmaps) {
      mNumMipmaps = maxMipmaps;
    }

    // allocate memory for all faces, all mipmaps

    int n = mNumMipmaps;
    int fc = (d <= 0 ? 6 : 1);
    
    MipLevel mipData;
    
    while (n >= 0) {
      
      int level = mNumMipmaps - n;

      size_t sz = desc.getBytesSizeFor(w, h, d, level, 1);

      int levelw = desc.getMipmappedDim(w, level);
      int levelh = desc.getMipmappedDim(h, level);
      int leveld = desc.getMipmappedDim(d, level);

#ifdef _DEBUG
      std::cout << "Mip level " << n << ": " << levelw << "x" << levelh << "x"
                << leveld << ", " << sz << " bytes" << std::endl;
#endif

      for (int i=0; i<fc; ++i) {

#ifdef _DEBUG
        std::cout << "  Allocate for face " << i << std::endl;
#endif

        mipData.data   = malloc(sz);
        mipData.width  = levelw;
        mipData.height = levelh;
        mipData.depth  = leveld;

        mFaces[i].push_back(mipData);
      }
      
      --n;
    }
  }
  
  Image::~Image() {
    for (int i=0; i<NUM_FACES; ++i) {
      for (size_t j=0; j<mFaces[i].size(); ++j) {
        free(mFaces[i][j].data);
      }
      mFaces[i].clear();
    }
  }

  void Image::clearMipmaps() {
#ifdef _DEBUG
    std::cout << "Clear image mipmaps" << std::endl;
#endif
    for (int i=0; i<NUM_FACES; ++i) {
      size_t sz = mFaces[i].size();
      while (sz > 1) {
        free(mFaces[i][--sz].data);
        mFaces[i].pop_back();
      }
      assert(mFaces[i].size() <= 1);
    }
    mNumMipmaps = 0;
  }

  void Image::buildMipmaps(int numMipmaps) {
#ifdef _DEBUG
    std::cout << "Build image mipmaps" << std::endl;
#endif
    if (mNumMipmaps > 0 || numMipmaps == 0) {
      return;
    }

    if (mDesc.isPacked() || mDesc.isCompressed()) {
      std::cerr << "Cannot build mipmaps for packed or compressed image format"
                << " (sorry i'm lazy)" << std::endl;
      return;
    }

    if (mDesc.isFloat() && mDesc.getBytesPerChannel() == 2) {
      std::cerr << "Cannot build mipmaps for a half float image format" << std::endl;
      return;
    }

    if (is3D()) {
      std::cerr << "Cannot build mipmaps for a 3D texture" << std::endl;
      return;
    }
    
    int maxMipmaps = mDesc.getMaxMipmaps(mMaxWidth, mMaxHeight, mMaxDepth);
    
    if (numMipmaps < 0) {
      numMipmaps = maxMipmaps;
    }

    if (numMipmaps > maxMipmaps) {
      numMipmaps = maxMipmaps;
    }
    
#ifdef _DEBUG
    std::cout << "Num mipmaps = " << numMipmaps << std::endl;
#endif

    size_t pixSize = mDesc.getBytesPerPixel();
    size_t chanSize = mDesc.getBytesPerChannel();
    int nChan = mDesc.getNumChannels();

    void (*mipmap_func)(void*, void*, void*, void*, int, void*);

    if (mDesc.isFloat()) {
      mipmap_func = mipmap_float;
    } else {
      if (chanSize == 1) {
        mipmap_func = mipmap_int8;
      } else if (chanSize == 2) {
        mipmap_func = mipmap_int16;
      } else {
        mipmap_func = mipmap_int32;
      }
    }

    for (int i=0; i<NUM_FACES; ++i) {

      if (mFaces[i].size() == 0) {
        break;
      }

      MipLevel ml;
      
      for (int level=1; level<=numMipmaps; ++level) {

        ml.data = malloc(mDesc.getBytesSizeFor(mMaxWidth, mMaxHeight, 1, level, 1));
        ml.width = mDesc.getMipmappedDim(mMaxWidth, level);
        ml.height = mDesc.getMipmappedDim(mMaxHeight, level);
        ml.depth = 1;


#ifdef _DEBUG
        std::cout << "Mipmap level " << level << " for face " << i << ": "
                  << ml.width << "x" << ml.height << ", "
                  << mDesc.getBytesSizeFor(mMaxWidth, mMaxHeight, 1, level, 1) << " bytes" << std::endl;
#endif

        size_t rowSize = mFaces[i][level-1].width * pixSize;

        unsigned char *p0, *p1, *p2, *p3, *pr;

        for (int y=0; y<ml.height; ++y) {

          pr = ((unsigned char*) ml.data) + (y * ml.width * pixSize);
          p0 = ((unsigned char*) mFaces[i][level-1].data) + (2 * y * rowSize);
          p1 = p0 + pixSize;
          p2 = p0 + rowSize;
          p3 = p2 + pixSize;

          for (int x=0; x<ml.width; ++x) {

            mipmap_func(p0, p1, p2, p3, nChan, pr);

            p0 += 2*pixSize;
            p1 += 2*pixSize;
            p2 += 2*pixSize;
            p3 += 2*pixSize;
            pr += pixSize;
          }
        }

        mFaces[i].push_back(ml);
      }
    }

    mNumMipmaps = numMipmaps;
    
#ifdef _DEBUG
    std::cout << "Done building mipmaps" << std::endl;
#endif
  }

  int Image::getWidth(int mipLevel, int face) const {
    if (face < 0 || face >= NUM_FACES) {
      return 0;
    }
    if (mipLevel < 0 || mipLevel >= int(mFaces[face].size())) {
      return 0;
    }
    return mFaces[face][mipLevel].width;  
  }
  
  int Image::getHeight(int mipLevel, int face) const {
    if (face < 0 || face >= NUM_FACES) {
      return 0;
    }
    if (mipLevel < 0 || mipLevel >= int(mFaces[face].size())) {
      return 0;
    }
    return mFaces[face][mipLevel].height;
  }
  
  int Image::getDepth(int mipLevel, int face) const {
    if (face < 0 || face >= NUM_FACES) {
      return 0;
    }
    if (mipLevel < 0 || mipLevel >= int(mFaces[face].size())) {
      return 0;
    }
    return mFaces[face][mipLevel].depth;
  }

  void* Image::getPixels(int mipLevel, int face) {
    if (face < 0 || face >= NUM_FACES) {
      return 0;
    }
    if (mipLevel < 0 || mipLevel >= int(mFaces[face].size())) {
      return 0;
    }
    return mFaces[face][mipLevel].data;  
  }

  void Image::scale(int w, int h, Image::ScaleMethod method) {
    
    if (mDesc.isPacked() || mDesc.isCompressed()) {
      std::cerr << "Cannot scale packed or compressed image format"
                << " (sorry i'm lazy)" << std::endl;
      return;
    }

    if (mDesc.isFloat() && mDesc.getBytesPerChannel() == 2) {
      std::cerr << "Cannot scale a half float image format" << std::endl;
      return;
    }

    if (is3D()) {
      std::cerr << "Cannot scale a 3D image" << std::endl;
      return;
    }
    
    size_t chanSize = mDesc.getBytesPerChannel();
    unsigned int pixSize = (unsigned int) mDesc.getBytesPerPixel();
    int numChan = mDesc.getNumChannels();
    
    PixelInitFunc initFunc;
    PixelAccumFunc accumFunc;
    
    if (mDesc.isFloat()) {
      initFunc = &pixInitT<float>;
      accumFunc = &pixAccumT<float>;
    } else {
      if (chanSize == 1) {
        initFunc = &pixInitT<unsigned char>;
        accumFunc = &pixAccumTClamped<unsigned char>;
      } else if (chanSize == 2) {
        initFunc = &pixInitT<unsigned short>;
        accumFunc = &pixAccumTClamped<unsigned short>;
      } else {
        initFunc = &pixInitT<unsigned long>;
        accumFunc = &pixAccumTClamped<unsigned long>;
      }
    }
  
    Filter *filter = 0;
    switch (method) {
    case NEAREST:
      filter = new BoxFilter();
      break;
    case LINEAR:
      filter = new LinearFilter();
      break;
    case CUBIC:
      filter = new CubicFilter();
      break;
    case LANCZOS:
      filter = new LanczosFilter();
    default:
      break;
    }
    
    if (!filter) {
      std::cerr << "Invalid filter specified" << std::endl;
      return;
    }
    
    unsigned int nmm = getNumMipmaps();
    clearMipmaps();
    
    for (int i=0; i<NUM_FACES; ++i) {
      
      if (mFaces[i].size() == 1) {
        
        void *src = mFaces[i][0].data;
        unsigned int width = mFaces[i][0].width;
        unsigned int height = mFaces[i][0].height;
        void *out = 0;
        
        if (w*height < h*width) {
          void *tmp = scaleHorizontal(src, width, height, numChan, pixSize, filter, w, initFunc, accumFunc);
          out = scaleVertical(tmp, w, height, numChan, pixSize, filter, h, initFunc, accumFunc);
          free(tmp);
          
        } else {
          void *tmp = scaleVertical(src, width, height, numChan, pixSize, filter, h, initFunc, accumFunc);
          out = scaleHorizontal(tmp, width, h, numChan, pixSize, filter, w, initFunc, accumFunc);
          free(tmp);
        }
        
        if (out) {
          free(mFaces[i][0].data);
          mFaces[i][0].data = out;
          mFaces[i][0].width = w;
          mFaces[i][0].height = h;
        }
      }
    }
    
    mMaxWidth = w;
    mMaxHeight = h;
    
    buildMipmaps(nmm);
  }
}
