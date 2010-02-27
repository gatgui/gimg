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

#include <gimg/format.h>

namespace gimg {
  
  PixelDesc::PixelDesc(PixelFormat fmt, PixelType type)
    :mFormat(fmt), mType(type) {
  }
  
  PixelDesc::PixelDesc(const PixelDesc &rhs)
    :mFormat(rhs.mFormat), mType(rhs.mType) {
  }
  
  PixelDesc::~PixelDesc() {
  }
  
  PixelDesc& PixelDesc::operator=(const PixelDesc &rhs) {
    mFormat = rhs.mFormat;
    mType = rhs.mType;
    return *this;
  }
  
  bool PixelDesc::isValid() const {
    if (isPacked()) {
      if (mType <= PT_INT_5_6_5) {
        return (mFormat == PF_RGB);
      } else {
        return (mFormat == PF_RGBA);
      }
    } else if (isCompressed()) {
      if (mType == PT_DXT1) {
        return (mFormat == PF_RGB || mFormat == PF_RGBA);
      } else {
        return (mFormat == PF_RGBA);
      }
    } else {
      return true;
    }
  }
  
  bool PixelDesc::isCompressed() const {
    return (mType >= PT_DXT1 && mType <= PT_3DC);
  }
  
  bool PixelDesc::isPacked() const {
    return (mType >= PT_INT_3_3_2 && mType <= PT_INT_10_10_10_2);
  }
  
  bool PixelDesc::isPlain() const {
    return (mType <= PT_FLOAT_32);
  }
  
  bool PixelDesc::isFloat() const {
    return (mType == PT_FLOAT_16 || mType == PT_FLOAT_32);
  }
  
  int PixelDesc::getMipmappedDim(int d, int mipLevel) const {
    int a = d >> mipLevel;
    return (a == 0) ? 1 : a;
  }
  
  int PixelDesc::getMaxMipmaps(int w, int h, int d) const {
    int max = (w > h) ? ((w > d) ? w : d) : ((h > d) ? h : d);
    int i = 0;
    while (max > 0){
      max >>= 1;
      i++;
    }
    // return i;
    --i;
    return (i <= 0 ? 0 : i);
  }
  
  int PixelDesc::getNumPixels(int w, int h, int d, int firstMip, int numMip) const {
    bool cube = (d <= 0);
    d = (cube ? 0 : d);
    w = getMipmappedDim(w, firstMip);
    h = getMipmappedDim(h, firstMip);
    d = getMipmappedDim(d, firstMip);
    numMip = (numMip <= 0 ? 1024 : numMip);
    int size = 0;
    while (numMip){
      size += w * h * d;
      w >>= 1;
      h >>= 1;
      d >>= 1;
      if (w + h + d == 0) break;
      if (w == 0) w = 1;
      if (h == 0) h = 1;
      if (d == 0) d = 1;
      --numMip;
    }
    return (cube ? 6 * size : size);
  }
  
  int PixelDesc::getNumBlocks(int w, int h, int d, int firstMip, int numMip) const {
    bool cube = (d <= 0);
    d = (cube ? 0 : 1);
    w = getMipmappedDim(w, firstMip);
    h = getMipmappedDim(h, firstMip);
    d = getMipmappedDim(d, firstMip);
    numMip = (numMip <= 0 ? 1024 : numMip); // 1024 should be enough to cover all mipmaps
    int size = 0;
    while (numMip) {
      size += ((w+3)>>2) * ((h+3)>>2) * d;
      w >>= 1;
      h >>= 1;
      d >>= 1;
      if (w + h + d == 0) break;
      if (w == 0) w = 1;
      if (h == 0) h = 1;
      if (d == 0) d = 1;
      --numMip;
    }
    return (cube ? 6 * size : size);
  }
  
  int PixelDesc::getNumChannels() const {
    static int numChannels[] = {1, 1, 1, 1, 1, 2, 3, 4};
    return numChannels[mFormat];
  }
  
  size_t PixelDesc::getBytesPerChannel() const {
    if (isPlain()) {
      static size_t bytesPerChannel[] = {1, 2, 4, 2, 4};
      return bytesPerChannel[mType];
    }
    return 0;
  }
  
  size_t PixelDesc::getBytesPerPixel() const {
    if (isPlain()) {
      return getNumChannels() * getBytesPerChannel();
    } else if (isPacked()) {
      static size_t bytesPerPixel[] = {1, 2, 2, 2, 4, 4};
      return bytesPerPixel[mType - PT_INT_3_3_2];
    }
    return 0;
  }
  
  size_t PixelDesc::getBytesPerBlock() const {
    if (mType < PT_DXT1 || mType > PT_3DC) {
      return 0;
    }
    return (mType == PT_DXT1 ? 8 : 16);
  }
  
  size_t PixelDesc::getBytesSizeFor(int w, int h, int d, int firstMipmap, int nMipmap) {
    if (isCompressed()) {
      return (getNumBlocks(w, h, d, firstMipmap, nMipmap) * getBytesPerBlock());
    } else {
      return (getNumPixels(w, h, d, firstMipmap, nMipmap) * getBytesPerPixel());
    }
  }
  
}

