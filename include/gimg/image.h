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

#ifndef __gimg_image_h_
#define __gimg_image_h_

#include <gimg/format.h>
#include <gcore/dmodule.h>
#include <gcore/callbacks.h>
#include <gcore/file.h>

namespace gimg {
  
  class GIMG_API Image {
    
    public:

      enum {
        CUBE = 0,
        FLAT = 1
      };
      
      enum CubeFace {
        X_PLUS = 0,
        Y_PLUS,
        Z_PLUS,
        X_MINUS,
        Y_MINUS,
        Z_MINUS,
        NUM_FACES
      };

      enum ScaleMethod {
        NEAREST = 0,
        LINEAR,
        CUBIC,
        LANCZOS
      };
      
    public:

      BEGIN_MODULE_INTERFACE ( Plugin )
        DEFINE_MODULE_SYMBOL0R ( int, numExtensions )
        DEFINE_MODULE_SYMBOL1R ( const char*, getExtension, int )
        DEFINE_MODULE_SYMBOL0R ( bool, canRead )
        DEFINE_MODULE_SYMBOL0R ( bool, canWrite )
        DEFINE_MODULE_SYMBOL1R ( Image*, readImage, const char* )
        DEFINE_MODULE_SYMBOL2R ( bool, writeImage, Image*, const char* )
      END_MODULE_INTERFACE

      static void LoadPlugins(const std::string &path);
      static void UnloadPlugins();
      static bool RegisterPlugin(Plugin *);
      static bool UnregisterPlugin(Plugin *);
      static Image* Read(const std::string &filepath, int numMips=-1);
      static bool Write(Image *img, const std::string &filepath);
      
    public:
      
      // depth <= 0 -> cube map
      // depth  = 1 -> 1D or 2D
      // depth  > 1 -> 3D

      // numMipmaps = 0     -> do not build mipmaps
      //            > 0 (n) -> build n additional mipmaps
      //            < 0     -> build all mipmaps to 1x1x1

      Image(PixelDesc desc, int w, int h, int d=1, int numMipmaps=0);
      virtual ~Image();
  
      void* getPixels(int mipLevel=0, int face=0);
      int getWidth(int mipLevel=0, int face=0) const;
      int getHeight(int mipLevel=0, int face=0) const;
      int getDepth(int mipLevel=0, int face=0) const;

      void clearMipmaps();
      void buildMipmaps(int numMipmaps);

      void scale(int w, int h, ScaleMethod method);
      
      inline bool is1D() const {
        return mMaxHeight==1 && mMaxDepth==1;
      }
      inline bool is2D() const {
        return mMaxHeight>1 && mMaxDepth==1;
      }
      inline bool is3D() const {
        return mMaxDepth>1;
      }
      inline bool isCube() const {
        return mMaxDepth<=0;
      }
      inline const PixelDesc& getPixelDesc() const {
        return mDesc;
      }
      inline int getNumMipmaps() const {
        return mNumMipmaps;
      }
    
    protected:
      
      int mMaxWidth;
      int mMaxHeight;
      int mMaxDepth;
      int mNumMipmaps;
      PixelDesc mDesc;

      struct MipLevel {
        void* data;
        int width;
        int height;
        int depth;
      };

      typedef std::vector<MipLevel> Face;

      Face mFaces[NUM_FACES];
      
    protected:
      
      struct ltstr {
        inline bool operator()(const char* s1, const char* s2) const {
#ifdef _WIN32
          return stricmp(s1, s2) < 0;
#else
          return strcasecmp(s1, s2) < 0;
#endif
        }
      };

      typedef std::map<const char*, Plugin*, ltstr> PluginMap;

      static std::vector<Plugin*> msPlugins;
      static PluginMap msReaders;
      static PluginMap msWriters; 

  };
  
  
  
}

#endif
