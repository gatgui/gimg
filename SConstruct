import glob
import excons
import excons.tools
from excons.tools import gl
from excons.tools import glut

SConscript("gcore/SConstruct")

prjs = [
  { "name"    : "gimg",
    "type"    : "sharedlib",
    "srcs"    : glob.glob("src/lib/*.cpp"),
    "defs"    : ["GIMG_EXPORTS"],
    "libs"    : ["gcore"],
    "incdirs" : ["include", "gcore/include"]
  },
  { "name"    : "gimg_bmp",
    "type"    : "dynamicmodule",
    "ext"     : ".ipl",
    "prefix"  : "share/plugins/gimg",
    "srcs"    : ["src/plugins/bmp/bmp.cpp"],
    "libs"    : ["gimg", "gcore"],
    "incdirs" : ["include", "gcore/include"]
  },
  { "name"    : "gimg_tga",
    "type"    : "dynamicmodule",
    "ext"     : ".ipl",
    "prefix"  : "share/plugins/gimg",
    "srcs"    : ["src/plugins/tga/tga.cpp"],
    "libs"    : ["gimg", "gcore"],
    "incdirs" : ["include", "gcore/include"]
  },
  { "name"    : "gimg_hdr",
    "type"    : "dynamicmodule",
    "ext"     : ".ipl",
    "prefix"  : "share/plugins/gimg",
    "srcs"    : ["src/plugins/hdr/hdr.cpp"],
    "libs"    : ["gimg", "gcore"],
    "incdirs" : ["include", "gcore/include"]
  },
  { "name"    : "tests",
    "type"    : "testprograms",
    "srcs"    : glob.glob("src/tests/*.cpp"),
    "libs"    : ["gimg", "gcore"],
    "deps"    : ["gimg_bmp", "gimg_tga", "gimg_hdr"],
    "incdirs" : ["include", "gcore/include"]
  },
  { "name"    : "view",
    "type"    : "program",
    "srcs"    : glob.glob("src/bin/*.cpp") + ["src/bin/glew.c"],
    "libs"    : ["gimg", "gcore"],
    "defs"    : ["GLEW_STATIC", "GLEW_BUILD"],
    "custom"  : [glut.Require],
    "deps"    : ["gimg_bmp", "gimg_tga", "gimg_hdr"],
    "incdirs" : ["include", "gcore/include"]
  }
]

env = excons.MakeBaseEnv()
excons.DeclareTargets(env, prjs)




