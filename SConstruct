import glob
import excons
import excons.tools
from excons.tools import gl
from excons.tools import glut

gcore_inc = None
gcore_lib = None

gcore_dir = ARGUMENTS.get("with-gcore", None)
if gcore_dir == None:
  gcore_inc = ARGUMENTS.get("with-gcore-inc", None)
  gcore_lib = ARGUMENTS.get("with-gcore-lib", None)
else:
  gcore_inc = gcore_dir + "/include"
  gcore_lib = gcore_dir + "/lib"
if gcore_inc == None or gcore_lib == None:
  print("### Warning ###")
  print("'gcore' library directory not specified")
  print("use 'with-gcore=' or 'with-gcore-inc' and 'with-gcore-lib' if needed")

prjs = [
  { "name"  : "gimg",
    "type"  : "sharedlib",
    "srcs"  : glob.glob("src/lib/*.cpp"),
    "defs"  : ["GIMG_EXPORTS"],
    "libs"  : ["gcore"],
    "incdirs": [gcore_inc],
    "libdirs": [gcore_lib]
  },
  { "name"  : "gimg_bmp",
    "type"  : "dynamicmodule",
    "ext"   : ".ipl",
    "prefix": "share/plugins/gimg",
    "srcs"  : ["src/plugins/bmp/bmp.cpp"],
    "libs"  : ["gimg", "gcore"],
    "incdirs": [gcore_inc],
    "libdirs": [gcore_lib]
  },
  { "name"  : "gimg_tga",
    "type"  : "dynamicmodule",
    "ext"   : ".ipl",
    "prefix": "share/plugins/gimg",
    "srcs"  : ["src/plugins/tga/tga.cpp"],
    "libs"  : ["gimg", "gcore"],
    "incdirs": [gcore_inc],
    "libdirs": [gcore_lib]
  },
  { "name"  : "gimg_hdr",
    "type"  : "dynamicmodule",
    "ext"   : ".ipl",
    "prefix": "share/plugins/gimg",
    "srcs"  : ["src/plugins/hdr/hdr.cpp"],
    "libs"  : ["gimg", "gcore"],
    "incdirs": [gcore_inc],
    "libdirs": [gcore_lib]
  },
  { "name"  : "tests",
    "type"  : "testprograms",
    "srcs"  : glob.glob("src/tests/*.cpp"),
    "libs"  : ["gimg", "gcore"],
    "deps"  : ["gimg_bmp", "gimg_tga", "gimg_hdr"],
    "incdirs": [gcore_inc],
    "libdirs": [gcore_lib]
  },
  { "name"  : "view",
    "type"  : "program",
    "srcs"  : glob.glob("src/bin/*.cpp") + ["src/bin/glew.c"],
    "libs"  : ["gimg", "gcore"],
    "defs"  : ["GLEW_STATIC", "GLEW_BUILD"],
    "custom": [glut.Require],
    "deps"  : ["gimg_bmp", "gimg_tga", "gimg_hdr"],
    "incdirs": [gcore_inc],
    "libdirs": [gcore_lib]
  }
]

env = excons.MakeBaseEnv()
excons.DeclareTargets(env, prjs)




