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

#include <gimg/color.h>
#include <cmath>

#define epsilon 0.000001f

namespace gimg {

Color::Color()
  : r(0.0f), g(0.0f), b(0.0f), a(1.0f) {
}

Color::Color(float v, float a)
  : r(v), g(v), b(v), a(a) {
}

Color::Color(float r, float g, float b, float a)
  : r(r), g(g), b(b), a(a) {
}

Color::Color(const Color &rhs)
  : r(rhs.r), g(rhs.g), b(rhs.b), a(rhs.a) {
}

Color::~Color() {
}

Color& Color::operator=(const Color &rhs) {
  if (this != &rhs) {
    r = rhs.r;
    g = rhs.g;
    b = rhs.b;
    a = rhs.a;
  }
  return *this;
}

Color& Color::operator+=(const Color &rhs) {
  r += rhs.r;
  g += rhs.g;
  b += rhs.b;
  // a?
  return *this;
}

Color& Color::operator-=(const Color &rhs) {
  r -= rhs.r;
  g -= rhs.g;
  b -= rhs.b;
  // a?
  return *this;
}

Color& Color::operator*=(const Color &rhs) {
  r *= rhs.r;
  g *= rhs.g;
  b *= rhs.b;
  // a?
  return *this;
}

Color& Color::operator*=(float f) {
  r *= f;
  g *= f;
  b *= f;
  // a?
  return *this;
}

Color& Color::operator/=(const Color &rhs) {
  if (rhs.r > epsilon) {
    r /= rhs.r;
  }
  if (rhs.g > epsilon) {
    g /= rhs.g;
  }
  if (rhs.b > epsilon) {
    b /= rhs.b;
  }
  // a?
  return *this;
}

Color& Color::operator/=(float d) {
  if (d > epsilon) {
    float id = 1.0f / d;
    r *= id;
    g *= id;
    b *= id;
  }
  // a?
  return *this;
}

bool Color::operator==(const Color &rhs) const
{
  if (fabsf(r-rhs.r) < epsilon && 
      fabsf(r-rhs.r) < epsilon &&
      fabsf(r-rhs.r) < epsilon) {
    return true;
  }
  return false;
}

bool Color::operator!=(const Color &rhs) const
{
  return !operator==(rhs);
}

void Color::premult()
{
  r *= a;
  g *= a;
  b *= a;
}

void Color::unpremult()
{
  if (a >= epsilon) {
    float ia = 1.0f / a;
    r *= ia;
    g *= ia;
    b *= ia;
  }
}

float Color::chroma() const {
  float M = std::max(r, std::max(g, b));
  float m = std::min(r, std::min(g, b));
  return (M - m);
}

float Color::luminance() const {
  // Rec. 709 (sRGB)
  //return 0.21f * r + 0.72f * g + 0.07 * b;
  // Rec. 601 NTSC
  return (0.3f * r + 0.59f * g + 0.11f * b);
}

float Color::intensity() const {
  static float aThird = 1.0f / 3.0f;
  return ((r + g + b) * aThird);
}

Color Color::toHSV() const {
  float M = std::max(r, std::max(g, b));
  float m = std::min(r, std::min(g, b));
  float C = M - m;
  
  float hue = 0.0f;
  float sat = 0.0f;
  float val = 0.0f;
  
  val = M;
  
  if (M >= epsilon) {
    sat = C / M;
  }
  
  if (sat >= epsilon) {
    
    if (M == r) {
      hue = (g - b) / C;
    } else if (M == g) {
      hue = (b - r) / C + 2.0f;
    } else {
      hue = (r - g) / C + 4.0f;
    }
    
    static float sNormalizeHue = 60.0f / 360.0f;
    
    hue *= sNormalizeHue;
    
    if (hue < 0.0f) {
      hue += 1.0f;
    }
  }
  
  return Color(hue, sat, val, a);
}

Color& Color::fromHSV(const Color &hsv) {
  float C = hsv.v * hsv.s;
  float h = hsv.h * 6.0f;
  float X = C * (1.0f - fabsf(fmodf(h, 2.0f) - 1));
  
  if (h < 1.0f) {
    r = C;
    g = X;
    b = 0.0f;
  } else if (h < 2.0f) {
    r = X;
    g = C;
    b = 0.0f;
  } else if (h < 3.0f) {
    r = 0.0f;
    g = C;
    b = X;
  } else if (h < 4.0f) {
    r = 0.0f;
    g = X;
    b = C;
  } else if (h < 5.0f) {
    r = X;
    g = 0.0f;
    b = C;
  } else {
    r = C;
    g = 0.0f;
    b = X;
  }
  
  float m = hsv.v - C;
  
  r += m;
  g += m;
  b += m;
  a = hsv.a;
  
  return *this;
}

Color Color::toHSL() const {
  float M = std::max(r, std::max(g, b));
  float m = std::min(r, std::min(g, b));
  float C = M - m;
  
  float hue = 0.0f;
  float sat = 0.0f;
  float lum = 0.0f;
  
  lum = 0.5f * (m + M);
  
  if (C >= epsilon) {
    if (lum <= 0.5f) {
      sat = C / (2.0f * lum);
    } else {
      sat = C / (2.0f * (1.0f - lum));
    }
    
    if (M == r) {
      hue = (g - b) / C;
    } else if (M == g) {
      hue = (b - r) / C + 2.0f;
    } else {
      hue = (r - g) / C + 4.0f;
    }
    
    static float sNormalizeHue = 60.0f / 360.0f;
    
    hue *= sNormalizeHue;
    
    if (hue < 0.0f) {
      hue += 1.0f;
    }
  }
  
  return Color(hue, sat, lum, a);
}

Color& Color::fromHSL(const Color &hsl) {
  float C;
  
  if (hsl.l <= 0.5f) {
    C = 2.0f * hsl.l * hsl.s;
  } else {
    C = (2.0f - 2.0f * hsl.l) * hsl.s;
  }
  
  float h = hsl.h * 6.0f;
  float X = C * (1.0f - fabsf(fmodf(h, 2.0f) - 1));
  
  if (h < 1.0f) {
    r = C;
    g = X;
    b = 0.0f;
  } else if (h < 2.0f) {
    r = X;
    g = C;
    b = 0.0f;
  } else if (h < 3.0f) {
    r = 0.0f;
    g = C;
    b = X;
  } else if (h < 4.0f) {
    r = 0.0f;
    g = X;
    b = C;
  } else if (h < 5.0f) {
    r = X;
    g = 0.0f;
    b = C;
  } else {
    r = C;
    g = 0.0f;
    b = X;
  }
  
  float m = hsl.l - 0.5f * C;
  
  r += m;
  g += m;
  b += m;
  a = hsl.a;
  
  return *this;
}

/**
  * back  : make this color black
  * white : make this color white
  * lift  : make black this color
  * gain  : make white this color
  */
Color Color::grade(const Color &black, const Color &white, const Color &lift, const Color &gain) {
  Color wbdiff(white);
  wbdiff -= black;
  
  Color gldiff(gain);
  gldiff -= lift;
  
  Color rv(*this);
  rv -= black;
  rv /= wbdiff;
  rv *= gldiff;
  rv += lift;
  
  return rv;
}

}

