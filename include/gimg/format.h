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

#ifndef __gimg_format_h_
#define __gimg_format_h_

#include <gimg/config.h>

namespace gimg {
  
  enum PixelFormat {
    PF_LUMINANCE = 0,
    PF_R,
    PF_G,
    PF_B,
    PF_A,
    PF_LUMINANCE_ALPHA,
    PF_RGB,
    PF_RGBA,
    PF_MAX
  };
  
  // RGBE ?
  enum PixelType {
    PT_INT_8 = 0,
    PT_INT_16,
    PT_INT_32,
    PT_FLOAT_16,
    PT_FLOAT_32,
    PT_INT_3_3_2,
    PT_INT_5_6_5,
    PT_INT_4_4_4_4,
    PT_INT_5_5_5_1,
    PT_INT_8_8_8_8,
    PT_INT_10_10_10_2,
    PT_DXT1,
    PT_DXT3,
    PT_DXT5,
    PT_3DC,
    PT_MAX
  };
  
  class GIMG_API PixelDesc {
    
    public:
      
      PixelDesc(PixelFormat fmt=PF_RGBA, PixelType type=PT_INT_8);
      PixelDesc(const PixelDesc &rhs);
      ~PixelDesc();
      
      PixelDesc& operator=(const PixelDesc &rhs);
      
      inline PixelType getType() const {return mType;}
      inline PixelFormat getFormat() const {return mFormat;}

      bool isValid() const;
      bool isCompressed() const;
      bool isPacked() const;
      bool isPlain() const;
      bool isFloat() const;
      
      // no format consideration
      int getMaxMipmaps(int w, int h, int d) const;
      int getMipmappedDim(int d, int mipLevel) const;
      int getNumPixels(int w, int h, int d, int firstMip, int numMip) const;
      int getNumBlocks(int w, int h, int d, int firstMip, int numMip) const;
      
      int getNumChannels() const;
      
      // plain only
      size_t getBytesPerChannel() const;
      
      // plain & packed only
      size_t getBytesPerPixel() const;
      // compressed only
      size_t getBytesPerBlock() const;
      
      size_t getBytesSizeFor(int w, int h, int d, int firstMipmap=0, int nMipmap=-1);
  
    protected:
      
      PixelFormat mFormat;
      PixelType mType;
  
  };
  
  
  
}



#endif
