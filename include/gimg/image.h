/*

Copyright (C) 2009, 2010  Gaetan Guidet

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
#include <gcore/functor.h>
#include <gcore/path.h>

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
      
      GCORE_BEGIN_MODULE_INTERFACE ( Plugin )
        GCORE_DEFINE_MODULE_SYMBOL0R ( int, numExtensions )
        GCORE_DEFINE_MODULE_SYMBOL1R ( const char*, getExtension, int )
        GCORE_DEFINE_MODULE_SYMBOL0R ( bool, canRead )
        GCORE_DEFINE_MODULE_SYMBOL0R ( bool, canWrite )
        GCORE_DEFINE_MODULE_SYMBOL1R ( Image*, readImage, const char* )
        GCORE_DEFINE_MODULE_SYMBOL2R ( bool, writeImage, Image*, const char* )
      GCORE_END_MODULE_INTERFACE
      
      static void LoadPlugins(const gcore::Path &path);
      static void UnloadPlugins();
      static bool RegisterPlugin(Plugin *);
      static bool UnregisterPlugin(Plugin *);
      static Image* Read(const gcore::Path &filepath, int numMips=-1);
      static bool Write(Image *img, const gcore::Path &filepath);
      
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
      
      typedef gcore::List<MipLevel> Face;
      
      Face mFaces[NUM_FACES];
      
    protected:
      
      typedef std::map<gcore::String, Plugin*> PluginMap;
      
      static gcore::List<Plugin*> msPlugins;
      static PluginMap msReaders;
      static PluginMap msWriters; 
  };
}

#endif
