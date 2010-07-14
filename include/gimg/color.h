/*

Copyright (C) 2010  Gaetan Guidet

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

#ifndef __gimg_color_h_
#define __gimg_color_h_

#include <gimg/config.h>

namespace gimg {
  
  
  class GIMG_API Color {
    public:
      
      union {
        float r;
        float h;
      };
      
      union {
        float g;
        float s;
      };
      
      union {
        float b;
        float v;
        float l;
      };
      
      float a;
    
    public:
      
      Color();
      Color(float v, float a=1.0f);
      Color(float r, float g, float b, float a=1.0f);
      Color(const Color &rhs);
      ~Color();
      
      Color& operator=(const Color &rhs);
      Color& operator+=(const Color &rhs);
      Color& operator-=(const Color &rhs);
      Color& operator*=(const Color &rhs);
      Color& operator*=(float f);
      Color& operator/=(const Color &rhs);
      Color& operator/=(float d);
      
      bool operator==(const Color &rhs) const;
      bool operator!=(const Color &rhs) const;
      
      void premult();
      void unpremult();
      
      float chroma() const;
      float luminance() const;
      float intensity() const;
      
      Color toHSV() const;
      Color& fromHSV(const Color &hsv);
      Color toHSL() const;
      Color& fromHSL(const Color &hsl);
      
      /**
        * back  : make this color black
        * white : make this color white
        * lift  : make black this color
        * gain  : make white this color
        */
      Color grade(const Color &black, const Color &white, const Color &lift, const Color &gain);
      
  };
}

inline gimg::Color operator+(const gimg::Color &c0, const gimg::Color &c1) {
  gimg::Color tmp(c0);
  tmp += c1;
  return tmp;
}

inline gimg::Color operator-(const gimg::Color &c0, const gimg::Color &c1) {
  gimg::Color tmp(c0);
  tmp -= c1;
  return tmp;
}

inline gimg::Color operator*(const gimg::Color &c0, const gimg::Color &c1) {
  gimg::Color tmp(c0);
  tmp *= c1;
  return tmp;
}

inline gimg::Color operator*(const gimg::Color &c, float f) {
  gimg::Color tmp(c);
  tmp *= f;
  return tmp;
}

inline gimg::Color operator*(float f, const gimg::Color &c) {
  gimg::Color tmp(c);
  tmp *= f;
  return tmp;
}

inline gimg::Color operator/(const gimg::Color &c0, const gimg::Color &c1) {
  gimg::Color tmp(c0);
  tmp /= c1;
  return tmp;
}

inline gimg::Color operator/(const gimg::Color &c, float d) {
  gimg::Color tmp(c);
  tmp /= d;
  return tmp;
}

inline std::ostream& operator<<(std::ostream &os, const gimg::Color &c) {
  os << "(" << c.h << ", " << c.s << ", " << c.v << ", " << c.a << ")";
  return os;
}

#endif
