#include <gimg/image.h>
#include <GL/glew.h>
#ifdef __APPLE__
# include <GLUT/glut.h>
#else
# include <GL/glut.h>
#endif
#include <cmath>

GLenum GLtypes[gimg::PT_MAX] = {
  GL_UNSIGNED_BYTE,           // PT_INT_8
  GL_UNSIGNED_SHORT,          // PT_INT_16
  GL_UNSIGNED_INT,            // PT_INT_32
  GL_HALF_FLOAT_ARB,          // PT_FLOAT_16
  GL_FLOAT,                   // PT_FLOAT_32
  GL_UNSIGNED_BYTE_3_3_2,     // PT_INT_3_3_2
  GL_UNSIGNED_SHORT_5_6_5,    // PT_INT_5_6_5
  GL_UNSIGNED_SHORT_4_4_4_4,  // PT_INT_4_4_4_4
  GL_UNSIGNED_SHORT_5_5_5_1,  // PT_INT_5_5_5_1
  GL_UNSIGNED_INT_8_8_8_8,    // PT_INT_8_8_8_8
  GL_UNSIGNED_INT_10_10_10_2, // PT_INT_10_10_10_2
  0,                          // PT_DXT1
  0,                          // PT_DXT2
  0,                          // PT_DXT3
  0                           // PT_3DC
};

GLenum GLformats[gimg::PF_MAX] = {
  GL_LUMINANCE,        // PF_LUMINANCE
  GL_RED,              // PF_R
  GL_GREEN,            // PF_G
  GL_BLUE,             // PF_B
  GL_ALPHA,            // PF_A
  GL_LUMINANCE_ALPHA,  // PF_LUMINANCE_ALPHA
  GL_RGB,              // PF_RGB
  GL_RGBA              // PF_RGBA
};

GLenum GLinternalFormats[gimg::PF_MAX][gimg::PT_MAX] = {
  { // PF_LUMINANCE
    GL_LUMINANCE,         // PT_INT_8
    GL_LUMINANCE16,       // PT_INT_16
    GL_LUMINANCE32UI_EXT, // PT_INT_32
    GL_LUMINANCE16F_ARB,  // PT_FLOAT_16
    GL_LUMINANCE32F_ARB,  // PT_FLOAT_32
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0
  },
  { // PF_R
    GL_RGB,         // PT_INT_8
    GL_RGB16,       // PT_INT_16
    GL_RGB32UI_EXT, // PT_INT_32
    GL_RGB16F_ARB,  // PT_FLOAT_16
    GL_RGB32F_ARB,  // PT_FLOAT_32
    GL_RGB,         // PT_INT_3_3_2
    GL_RGB,         // PT_INT_5_6_5
    GL_RGB,         // PT_INT_4_4_4_4
    GL_RGB,         // PT_INT_5_5_5_1
    GL_RGB,         // PT_INT_8_8_8_8
    GL_RGB16,       // PT_INT_10_10_10_2
    0,
    0,
    0,
    0
  },
  { // PF_G
    GL_RGB,         // PT_INT_8
    GL_RGB16,       // PT_INT_16
    GL_RGB32UI_EXT, // PT_INT_32
    GL_RGB16F_ARB,  // PT_FLOAT_16
    GL_RGB32F_ARB,  // PT_FLOAT_32
    GL_RGB,         // PT_INT_3_3_2
    GL_RGB,         // PT_INT_5_6_5
    GL_RGB,         // PT_INT_4_4_4_4
    GL_RGB,         // PT_INT_5_5_5_1
    GL_RGB,         // PT_INT_8_8_8_8
    GL_RGB16,       // PT_INT_10_10_10_2
    0,
    0,
    0,
    0
  },
  { // PF_B
    GL_RGB,         // PT_INT_8
    GL_RGB16,       // PT_INT_16
    GL_RGB32UI_EXT, // PT_INT_32
    GL_RGB16F_ARB,  // PT_FLOAT_16
    GL_RGB32F_ARB,  // PT_FLOAT_32
    GL_RGB,         // PT_INT_3_3_2
    GL_RGB,         // PT_INT_5_6_5
    GL_RGB,         // PT_INT_4_4_4_4
    GL_RGB,         // PT_INT_5_5_5_1
    GL_RGB,         // PT_INT_8_8_8_8
    GL_RGB16,       // PT_INT_10_10_10_2
    0,
    0,
    0,
    0
  },
  { // PF_A
    GL_ALPHA,         // PT_INT_8
    GL_ALPHA16,       // PT_INT_16
    GL_ALPHA32UI_EXT, // PT_INT_32
    GL_ALPHA16F_ARB,  // PT_FLOAT_16
    GL_ALPHA32F_ARB,  // PT_FLOAT_32
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0
  },
  { // PF_LUMINANCE_ALPHA
    GL_LUMINANCE_ALPHA,         // PT_INT_8
    GL_LUMINANCE16_ALPHA16,     // PT_INT_16
    GL_LUMINANCE_ALPHA32UI_EXT, // PT_INT_32
    GL_LUMINANCE_ALPHA16F_ARB,  // PT_FLOAT_16
    GL_LUMINANCE_ALPHA32F_ARB,  // PT_FLOAT_32
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0
  },
  { // PF_RGB
    GL_RGB,         // PT_INT_8
    GL_RGB16,       // PT_INT_16
    GL_RGB32UI_EXT, // PT_INT_32
    GL_RGB16F_ARB,  // PT_FLOAT_16
    GL_RGB32F_ARB,  // PT_FLOAT_32
    GL_RGB,
    GL_RGB,
    GL_RGB,
    GL_RGB,
    GL_RGB,
    GL_RGB16,
    0,
    0,
    0,
    0
  },
  { // PF_RGBA
    GL_RGBA,         // PT_INT_8
    GL_RGBA16,       // PT_INT_16
    GL_RGBA32UI_EXT, // PT_INT_32
    GL_RGBA16F_ARB,  // PT_FLOAT_16
    GL_RGBA32F_ARB,  // PT_FLOAT_32
    GL_RGBA,
    GL_RGBA,
    GL_RGBA,
    GL_RGBA,
    GL_RGBA,
    GL_RGBA16,
    0,
    0,
    0,
    0
  }
};


static const char *gVShader =
"void main(void) {\n"
"  gl_Position = ftransform();\n"
"  gl_TexCoord[0] = gl_MultiTexCoord0;\n"
"}\n"
"\n";

static const char *gPShader =
"uniform float gamma;\n"
"uniform sampler2D image;\n"
"\n"
"void main(void) {\n"
"  vec3 C  = texture2D(image, gl_TexCoord[0].st).rgb;\n"
"  gl_FragColor.r = pow(C.r, gamma);\n"
"  gl_FragColor.g = pow(C.g, gamma);\n"
"  gl_FragColor.b = pow(C.b, gamma);\n"
"}\n"
"\n";

static char gLog[4096];

struct Vertex {
  float x, y, z;
  float u, v;
};

enum DragOperation {
  DO_MOVE = 0,
  DO_ZOOM = 1,
  DO_NONE
};

struct ApplicationData {
  gimg::Image *img;
  GLuint tex;

  float offx;
  float offy;
  float zoom;
  Vertex quad[4];

  int width;
  int height;
  DragOperation dragop;
  int lastx;
  int lasty;
  
  float gamma;
  int miplevel;

  bool canUseProg;
  bool useProg;
  GLuint prog;
  GLuint vs;
  GLuint ps;
};

ApplicationData *gData = 0;

GLenum TexTarget = GL_TEXTURE_2D; // GL_TEXTURE_RECTANGLE_ARB

void updateTexture() {
#ifdef _DEBUG
  std::cout << "updateTexture" << std::endl;
#endif
  gimg::PixelDesc desc = gData->img->getPixelDesc();
    
  GLenum ifmt = GLinternalFormats[desc.getFormat()][desc.getType()];
  GLenum fmt  = GLformats[desc.getFormat()];
  GLenum typ  = GLtypes[desc.getType()];
  
#ifdef _DEBUG
  std::cout << gData->img->getWidth(gData->miplevel) << "x"
            << gData->img->getHeight(gData->miplevel) << std::endl;
  fprintf(stdout, "Pixels @ 0x%p\n", gData->img->getPixels(gData->miplevel));
#endif

  glBindTexture(TexTarget, gData->tex);
  
#ifdef _DEBUG
  std::cout << "glTexImage2D" << std::endl;
#endif
  
  // IMPORTANT NOTE: even for opengl the scanline size must be a multiple of 4
  // -> width = 1323, with GL_RGB and GL_UNSIGNED_BYTE will fail to load (crash)
  //    as 1323 * 3 is not divideable by 4... need padding (just like BMP files)
  // -> use GL_UNPACK_ALIGNMENT
  
  GLint align;
  glGetIntegerv(GL_UNPACK_ALIGNMENT, &align);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  
  glTexImage2D(TexTarget, 0, ifmt,
               gData->img->getWidth(gData->miplevel),
               gData->img->getHeight(gData->miplevel),
               0, fmt, typ, gData->img->getPixels(gData->miplevel));

  glPixelStorei(GL_UNPACK_ALIGNMENT, align);

#ifdef _DEBUG
  std::cout << "Done updateTexture" << std::endl;
#endif
}

void loadTexture() {
#ifdef _DEBUG
  std::cout << "loadTexture" << std::endl;
#endif
  glGenTextures(1, &(gData->tex));
  glBindTexture(TexTarget, gData->tex);
  glTexParameteri(TexTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(TexTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  updateTexture();
#ifdef _DEBUG
  std::cout << "Done loadTexture" << std::endl;
#endif
}

int compileStatus(GLuint sdr) {
  GLint r = 0;
  glGetShaderiv(sdr, GL_COMPILE_STATUS, &r);
  if (r != 1) {
    std::cout << "*** Error ***" << std::endl;
    GLsizei l = 0;
    glGetShaderInfoLog(sdr, 4096, &l, gLog);
    gLog[l] = '\0';
    std::cout << gLog << std::endl;
  }
  return r;
}

int linkStatus(GLuint prg) {
  GLint r = 0;
  glGetProgramiv(prg, GL_LINK_STATUS, &r);
  if (r != 1) {
    std::cout << "*** Error ***" << std::endl;
    GLsizei l = 0;
    glGetProgramInfoLog(prg, 4096, &l, gLog);
    gLog[l] = '\0';
    std::cout << gLog << std::endl;
  }
  return r;
}

void loadShaders() {
  
  GLint vslen = (GLint) strlen(gVShader);
  GLint pslen = (GLint) strlen(gPShader);

  gData->canUseProg = false;
  gData->useProg = false;
  
  gData->vs = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(gData->vs, 1, &gVShader, &vslen);
  glCompileShader(gData->vs);
  
  if (compileStatus(gData->vs)) {
  
    gData->ps = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(gData->ps, 1, &gPShader, &pslen);
    glCompileShader(gData->ps);
    
    if (compileStatus(gData->ps)) {
  
      gData->prog = glCreateProgram();
      glAttachShader(gData->prog, gData->vs);
      glAttachShader(gData->prog, gData->ps);
      glLinkProgram(gData->prog);
      
      if (linkStatus(gData->prog)) {
  
        glUseProgram(gData->prog);
        glUniform1f(glGetUniformLocation(gData->prog, "image"), gData->tex);
        glUniform1f(glGetUniformLocation(gData->prog, "gamma"), gData->gamma);
        glUseProgram(0);
        
        gData->canUseProg = true;
        gData->useProg = true;
      }
    }
  }
}

void startup() {
  gData->gamma = 1.0f / 2.2f;
  glEnable(TexTarget);
  glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
  loadTexture();
  loadShaders();
}

void cleanup() {
  glDeleteTextures(1, &(gData->tex));
  glDeleteShader(gData->vs);
  glDeleteShader(gData->ps);
  glDeleteProgram(gData->prog);
  delete gData->img;
  gimg::Image::UnloadPlugins();
}

void zoom(float z) {
  float dz  = z / gData->zoom;
  float cx  = float(gData->width) * 0.5f;
  float cy  = float(gData->height) * 0.5f;
  float oiw = gData->img->getWidth(gData->miplevel) * gData->zoom;
  float oih = gData->img->getHeight(gData->miplevel) * gData->zoom;
  float oox = gData->offx;
  float ooy = gData->offy;
  float icx = oox + (0.5f * oiw);
  float icy = ooy + (0.5f * oih);
  float dcx = (icx - cx) * dz;
  float dcy = (icy - cy) * dz;
  gData->zoom = z;
  float niw = gData->img->getWidth(gData->miplevel) * gData->zoom;
  float nih = gData->img->getHeight(gData->miplevel) * gData->zoom;
  float ncx = dcx + cx;
  float ncy = dcy + cy;
  gData->offx = ncx - (niw * 0.5f);
  gData->offy = ncy - (nih * 0.5f);
}

void resize(int w, int h) {
  float cx  = float(gData->width) * 0.5f;
  float cy  = float(gData->height) * 0.5f;
  float iw  = gData->img->getWidth(gData->miplevel) * gData->zoom;
  float ih  = gData->img->getHeight(gData->miplevel) * gData->zoom;
  float icx = gData->offx + (0.5f * iw);
  float icy = gData->offy + (0.5f * ih);
  float dcx = (icx - cx);
  float dcy = (icy - cy);
  float ncx = w * 0.5f;
  float ncy = h * 0.5f;
  icx = ncx + dcx;
  icy = ncy + dcy;
  gData->offx = icx - (0.5f * iw);
  gData->offy = icy - (0.5f * ih);
  gData->width = w;
  gData->height = h;
}

void center() {
  float w  = float(gData->width);
  float h  = float(gData->height);
  float iw = gData->img->getWidth(gData->miplevel) * gData->zoom;
  float ih = gData->img->getHeight(gData->miplevel) * gData->zoom;
  gData->offx = 0.5f * (w - iw);
  gData->offy = 0.5f * (h - ih);
}

void display() {
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
  glPushMatrix();
  glBindTexture(TexTarget, gData->tex);
  if (gData->useProg) {
    glUseProgram(gData->prog);
  }
  glTranslatef(gData->offx, gData->offy, 0.0f);
  glScalef(gData->zoom, gData->zoom, 1.0f);
  glBegin(GL_QUADS);
  for (int i=0; i<4; ++i) {
    glTexCoord2fv(&(gData->quad[i].u));
    glVertex3fv(&(gData->quad[i].x));
  }
  glEnd();
  glBindTexture(TexTarget, 0);
  if (gData->useProg) {
    glUseProgram(0);
  }
  glPopMatrix();
  glutSwapBuffers();
}

void reshape(int w, int h) {
  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0f, w, 0.0f, h, -1.0f, 1.0f);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  resize(w, h);
}

void keyboard(unsigned char key, int, int) {
  switch (key) {
  case 27:
    ::exit(0);
  case '+':
    zoom(gData->zoom * 2.0f);
    glutPostRedisplay();
    break;
  case '-':
    zoom(gData->zoom * 0.5f);
    glutPostRedisplay();
    break;
  case 'c':
    center();
    glutPostRedisplay();
    break;
  case 'm':
    if (gData->miplevel > 0) {
      gData->miplevel -= 1;
      std::cout << "Mip level " << gData->miplevel << " "
                << gData->img->getWidth(gData->miplevel) << "x"
                << gData->img->getHeight(gData->miplevel) << std::endl;
      updateTexture();
      glutPostRedisplay();
    }
    break;
  case 'M':
    if (gData->miplevel < gData->img->getNumMipmaps()) {
      gData->miplevel += 1;
      std::cout << "Mip level " << gData->miplevel << " "
                << gData->img->getWidth(gData->miplevel) << "x"
                << gData->img->getHeight(gData->miplevel) << std::endl;
      updateTexture();
      glutPostRedisplay();
    }
    break;
  case 'r':
    zoom(1.0f);
    glutPostRedisplay();
    break;
  case 'g':
    if (gData->canUseProg) {
      gData->useProg = !gData->useProg;
    }
    glutPostRedisplay();
    break;
  default:
    break;
  }
}

void mouse_click(int button, int state, int x, int y) {
  if (state == GLUT_DOWN) {
    if (button == GLUT_LEFT_BUTTON) {
      gData->lastx = x;
      gData->lasty = y;
      gData->dragop = DO_MOVE;
    } else if (button == GLUT_RIGHT_BUTTON) {
      gData->lastx = x;
      gData->lasty = y;
      gData->dragop = DO_ZOOM;
    }
  } else {
    gData->dragop = DO_NONE;
  }
}

void mouse_move(int, int) {
}

void mouse_drag(int x, int y) {
  if (gData->dragop == DO_MOVE) {
    gData->offx += float(x - gData->lastx);
    gData->offy -= float(y - gData->lasty);
    glutPostRedisplay();
  } else if (gData->dragop == DO_ZOOM) {
    int dy = gData->lasty - y;
    int dx = x - gData->lastx;
    int d = (abs(dx) > abs(dy) ? dx : dy);
    float nw = float(gData->width) + (2.0f * d);
    float s = nw / float(gData->width);
    zoom(gData->zoom * s);
    glutPostRedisplay();
  }
  gData->lastx = x;
  gData->lasty = y;
}

void usage() {
  std::cout << "Usage: imgview <filename> [OPTIONS]" << std::endl;
  std::cout << std::endl;
  std::cout << "OPTIONS:" << std::endl;
  std::cout << std::endl;
  std::cout << "  -p <path>  : plugin search directory  [\"./share/plugins/gimg\"]" << std::endl;
  std::cout << "  -s <scale> : image scale in percent   [100]" << std::endl;
  std::cout << "  -w <width> : image scaled width" << std::endl;
  std::cout << "  -h <height>: image scaled height" << std::endl;
  std::cout << "  -f <filter>: scale filter             [linear]" << std::endl;
  std::cout << "               box|linear|cubic|lanczos" << std::endl;
  std::cout << std::endl;
  std::cout << "NOTE: -w/-h takes precedence on -s" << std::endl;
}

int main(int argc, char **argv) {

  if (argc < 2) {
    usage();
    return -1;
  }

  std::string filePath = "";
  std::string pluginPath = "./share/plugins/gimg";
  double scale = 1.0;
  unsigned int w = 0;
  unsigned int h = 0;
  gimg::Image::ScaleMethod filter = gimg::Image::NEAREST;
  
  int arg = 1;
  
  while (arg < argc) {
    if (strcmp(argv[arg], "-p") == 0) {
      ++arg;
      if (arg >= argc) {
        std::cerr << "Missing option argument" << std::endl;
        usage();
        return -1;
      }
      pluginPath = argv[arg++];
      
    } else if (strcmp(argv[arg], "-s") == 0) {
      ++arg;
      int percent = 0;
      if (sscanf(argv[arg++], "%d", &percent) != 1) {
        std::cerr << "Invalid option argument (-r)" << std::endl;
        usage();
        return -1;
      }
      scale = double(percent) / 100.0;
      std::cout << "Image scale: " << scale << std::endl;
    
    } else if (strcmp(argv[arg], "-w") == 0) {
      ++arg;
      if (sscanf(argv[arg++], "%u", &w) != 1) {
        std::cerr << "Invalid option argument (-w)" << std::endl;
        usage();
        return -1;
      }
      std::cout << "Image wanted width: " << w << std::endl;
    
    } else if (strcmp(argv[arg], "-h") == 0) {
      ++arg;
      if (sscanf(argv[arg++], "%u", &h) != 1) {
        std::cerr << "Invalid option argument (-h)" << std::endl;
        usage();
        return -1;
      }
      std::cout << "Image wanted height: " << h << std::endl;
      
    } else if (strcmp(argv[arg], "-f") == 0) {
      ++arg;
      if (!strcmp(argv[arg], "box")) {
        filter = gimg::Image::NEAREST;
      } else if (!strcmp(argv[arg], "linear")) {
        filter = gimg::Image::LINEAR;
      } else if (!strcmp(argv[arg], "cubic")) {
        filter = gimg::Image::CUBIC;
      } else if (!strcmp(argv[arg], "lanczos")) {
        filter = gimg::Image::LANCZOS;
      } else {
        std::cerr << "Invalid option argument (-f)" << std::endl;
        usage();
        return -1;
      }
      ++arg;
      std::cout << "Scale filter: " << filter << std::endl;
    
    } else {
      if (filePath.length() > 0) {
        std::cerr << "Unrecognize option: \"" << argv[arg] << "\"" << std::endl;
        usage();
        return -1;
      }
      filePath = argv[arg++];
      std::cout << "Input file name: " << filePath << std::endl;
    }
  }
  

  gimg::Image::LoadPlugins(pluginPath.c_str());

  gimg::Image *img = gimg::Image::Read(filePath.c_str());

  if (!img) {
    std::cout << "Could not read image file" << std::endl;
    gimg::Image::UnloadPlugins();
    
  } else {
    
    ApplicationData data;
    gData = &data;
    
    // some size get not display properly... 1323x1052
    
    if (scale < 1 || scale > 1 || w != 0 || h != 0) {
      unsigned int nw, nh;
      if (w == 0 && h == 0) {
        nw = (unsigned int) ceil(double(img->getWidth()) * scale);
        nh = (unsigned int) ceil(double(img->getHeight()) * scale);
      } else if (w != 0) {
        nw = w;
        scale = double(w) / img->getWidth();
        nh = (unsigned int) ceil(double(img->getHeight() * scale));
      } else {
        nh = h;
        scale = double(h) / img->getHeight();
        nw = (unsigned int) ceil(double(img->getWidth() * scale));
      }
      img->scale(nw, nh, filter);
    }
    
    if (img->getNumMipmaps() == 0) {
      img->buildMipmaps(-1);
    }
  
    data.img = img;
    data.zoom = 1.0f;
    data.offx = 0.0f;
    data.offy = 0.0f;
    data.dragop = DO_NONE;
    data.miplevel = 0;
    data.width = data.img->getWidth(gData->miplevel);
    data.height = data.img->getHeight(gData->miplevel);
    data.quad[0].x = 0.0f;
    data.quad[0].y = 0.0f;
    data.quad[0].z = 0.0f;
    data.quad[1].x = data.width;
    data.quad[1].y = 0.0f;
    data.quad[1].z = 0.0f;
    data.quad[2].x = data.width;
    data.quad[2].y = data.height;
    data.quad[2].z = 0.0f;
    data.quad[3].x = 0.0f;
    data.quad[3].y = data.height;
    data.quad[3].z = 0.0f;  
    data.quad[0].u = 0.0f;
    data.quad[0].v = 1.0f;
    data.quad[1].u = 1.0f;
    data.quad[1].v = 1.0f;
    data.quad[2].u = 1.0f;
    data.quad[2].v = 0.0f;
    data.quad[3].u = 0.0f;
    data.quad[3].v = 0.0f;
  
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA|GLUT_DEPTH|GLUT_DOUBLE);
    glutInitWindowPosition(50, 50);
    glutInitWindowSize(data.img->getWidth(gData->miplevel), data.img->getHeight(gData->miplevel));
    std::string title = "Image Viewer: " + filePath;
    glutCreateWindow(title.c_str());
    glewInit();
    startup();
    atexit(cleanup);
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse_click);
    glutMotionFunc(mouse_drag);
    glutPassiveMotionFunc(mouse_move);
    glutMainLoop();
  }
}
