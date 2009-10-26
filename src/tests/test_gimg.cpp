#include <gimg/image.h>

using namespace std;
using namespace gimg;

static std::string FormatString[]  = {
  "PF_LUMINANCE",
  "PF_R",
  "PF_G",
  "PF_B",
  "PF_A",
  "PF_LUMINANCE_ALPHA",
  "PF_RGB",
  "PF_RGBA",
  "PF_DEPTH"
};

static std::string TypeString[] = {
  "PT_INT_8",
  "PT_INT_16",
  "PT_INT_32",
  "PT_FLOAT_16",
  "PT_FLOAT_32",
  "PT_INT_3_3_2",
  "PT_INT_5_6_5",
  "PT_INT_4_4_4_4",
  "PT_INT_5_5_5_1",
  "PT_INT_8_8_8_8",
  "PT_INT_10_10_10_2",
  "PT_DXT1",
  "PT_DXT3",
  "PT_DXT5",
  "PT_3DC"
};

// ---

struct RGBA {
  unsigned char r, g, b, a;
  RGBA():r(0),g(0),b(0),a(255) {
  }
  RGBA(unsigned char inR, unsigned char inG, unsigned char inB, unsigned char inA=255)
    :r(inR), g(inG), b(inB), a(inA) {
  }
  RGBA(const RGBA &rhs)
    :r(rhs.r), g(rhs.g), b(rhs.b), a(rhs.a) {
  }
  ~RGBA() {
  }
  bool operator==(const RGBA &rhs) const {
    return ((r==rhs.r) && (g==rhs.g) && (b==rhs.b) && (a==rhs.a));
  }
  bool operator!=(const RGBA &rhs) const {
    return !operator==(rhs);
  }
  RGBA& operator=(const RGBA &rhs) {
    r = rhs.r;
    g = rhs.g;
    b = rhs.b;
    a = rhs.a;
    return *this;
  }
  RGBA& operator*=(unsigned char s) {
    r *= s;
    g *= s;
    b *= s;
    a *= s;
    return *this;
  }
  RGBA& operator+=(const RGBA &rhs) {
    r += rhs.r;
    g += rhs.g;
    b += rhs.b;
    a += rhs.a;
    return *this;
  }
  RGBA& operator-=(const RGBA &rhs) {
    r -= rhs.r;
    g -= rhs.g;
    b -= rhs.b;
    a -= rhs.a;
    return *this;
  }
  unsigned short toR5G6B5() const {
    unsigned char sR = r >> 3;
    unsigned char sG = g >> 4;
    unsigned char sB = b >> 3;
    return ((sR << 11) | (sG << 5) | sB);
  }
  void fromR5G6B5(unsigned short v) {
    r = (v & 0xF800) >> 11;
    g = (v & 0x07E0) >> 5;
    b = (v & 0x001F);
    r <<= 3;
    g <<= 4;
    b <<= 3;
  }
};

ostream& operator<<(ostream &os, const RGBA &c) {
  os << "Color(" << int(c.r) << ", " << int(c.g) << ", " << int(c.b) << ", " << int(c.a) << ")";
  return os;
}

RGBA operator+(const RGBA &c0, const RGBA &c1) {
  RGBA res(c0);
  return res += c1;
}

RGBA operator-(const RGBA &c0, const RGBA &c1) {
  RGBA res(c0);
  return res -= c1;
}

RGBA operator*(const RGBA &c, unsigned char n) {
  RGBA res(c);
  return res *= n;
}

RGBA operator*(unsigned char n, const RGBA &c) {
  RGBA res(c);
  return res *= n;
}

// ---

/*
void decodeDXT1(unsigned char *src, unsigned char *dst, int w, int h, int x, int y) {
  // DXT1 --> 8 bytes block
  //          2 * 16 bit color (4 bytes) (5:6:5)
  //          4x4 2bits table (4 bytes)
  RGBA c0, c1, c2, c3;
  
  unsigned short *cur = (unsigned short*)src;
  
  c0.fromR5G6B5(*cur++);
  c1.fromR5G6B5(*cur++);
  
  // now the table 4x4x2bits
  // x == column, y == row
  int xstep = 4;
  int ystep = w * 4; // if dest is RGBA
  
  
  cout << c0 << endl;
  cout << c1 << endl;
}
*/

int main(int, char**) {
  
  RGBA c0(128, 0, 57);
  RGBA c1(78, 67, 6);
  
  unsigned short c2 = c0.toR5G6B5();
  unsigned short c3 = c1.toR5G6B5();
  
  RGBA c4;
  RGBA c5;
  
  c4.fromR5G6B5(c2);
  c5.fromR5G6B5(c3);
  
  cout << c0 << " -> " << c2 << " -> " << c4 << endl;
  cout << c1 << " -> " << c3 << " -> " << c5 << endl;
  
  //unsigned short data[2] = {c2, c3};
  //decodeDXT1((unsigned char*)data, NULL, 0, 0, 0, 0);
  
  gimg::PixelDesc desc(gimg::PF_RGB, gimg::PT_FLOAT_32);
  
  gimg::Image img0(desc, 1024, 1024, 1, -1); // auto ?
  gimg::Image img1(desc, 1024, 1024, 1, 4);
  gimg::Image img2(desc, 1024, 1024, 0, 2);
  gimg::Image img3(desc, 1024, 1024, 1, 0);

  std::cout << "MipLevel 3 / " << img0.getNumMipmaps()
            << ": " << img0.getWidth(3)
            << "x" << img0.getHeight(3)
            << "x" << img0.getDepth(3) << std::endl;

  //PixelDesc desc;
  
  int w = 512;
  int h = 512;
  int d = 1;
  
  for (int i=0; i<PF_MAX; ++i) {
    for (int j=0; j<PT_MAX; ++j) {
      desc = PixelDesc((PixelFormat)i, (PixelType)j);
      cout << "PixelDesc: " << FormatString[i] << " - " << TypeString[j] << endl;
      if (desc.isValid()) {
        cout << "  Plain: " << (desc.isPlain() ? "true" : "false") << endl;
        cout << "  Packed: " << (desc.isPacked() ? "true" : "false") << endl;
        cout << "  Compressed: " << (desc.isCompressed() ? "true" : "false") << endl;
        cout << "  Float: " << (desc.isFloat() ? "true" : "false") << endl;
      } else {
        cout << "  INVALID" << endl;
      }
    }
  }
  
  desc = PixelDesc(PF_RGBA, PT_FLOAT_32);
  cout << "For size: " << w << "x" << h << "x" << d << endl;
  cout << "  Max mipmaps = " << desc.getMaxMipmaps(w, h, d) << endl;
  cout << "  Num pixels = " << desc.getNumPixels(w, h, d, 0, 6) << endl;
  cout << "  Num blocks = " << desc.getNumBlocks(w, h, d, 0, 6) << endl;
  cout << "  Bytes size = " << desc.getBytesSizeFor(w, h, d, 0, -1) << endl;
  for (int l=0; l<=desc.getMaxMipmaps(w,h,d); ++l) {
    cout << "  Level " << l << endl;
    cout << "    Num pixels = " << desc.getNumPixels(w, h, d, l, 1) << endl;
    cout << "    Num blocks = " << desc.getNumBlocks(w, h, d, l, 1) << endl;
    cout << "    Bytes size = " << desc.getBytesSizeFor(w, h, d, l, 1) << endl;
    cout << "    w = " << desc.getMipmappedDim(w, l) << endl;
    cout << "    h = " << desc.getMipmappedDim(h, l) << endl;
    cout << "    d = " << desc.getMipmappedDim(d, l) << endl;
  }
  
  return 0;
}

