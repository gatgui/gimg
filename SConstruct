import glob
import os

initsubs = False

gcore_prefix = ARGUMENTS.get("with-gcore", None)
if not gcore_prefix:
  gcore_inc = ARGUMENTS.get("with-gcore-inc", None)
  gcore_lib = ARGUMENTS.get("with-gcore-lib", None)
else:
  gcore_inc = gcore_prefix + "/include"
  gcore_lib = gcore_prefix + "/lib"

try:
  import excons
  if len(glob.glob("gcore/*")) == 0 and not (gcore_inc and gcore_lib):
    initsubs = True
except:
  initsubs = True

if initsubs:
  import subprocess
  subprocess.Popen("git submodule init", shell=True).communicate()
  subprocess.Popen("git submodule update", shell=True).communicate()
  
  import excons

import excons.tools
from excons.tools import gl
from excons.tools import glut

build_gcore = False

if gcore_inc is None or gcore_lib is None:
  gcore_inc = "gcore/include"
  build_gcore = True

if build_gcore:
  # Forces shared library build or gcore
  ARGUMENTS["static"] = "0"
  SConscript("gcore/SConstruct")

libdirs = [] if gcore_lib is None else [gcore_lib]

prjs = [
  { "name"    : "gimg",
    "type"    : "sharedlib",
    "srcs"    : glob.glob("src/lib/*.cpp"),
    "defs"    : ["GIMG_EXPORTS"],
    "libs"    : ["gcore"],
    "incdirs" : ["include", gcore_inc],
    "libdirs" : libdirs
  },
  { "name"    : "gimg_bmp",
    "type"    : "dynamicmodule",
    "ext"     : ".ipl",
    "prefix"  : "share/plugins/gimg",
    "srcs"    : ["src/plugins/bmp/bmp.cpp"],
    "libs"    : ["gimg", "gcore"],
    "incdirs" : ["include", gcore_inc],
    "libdirs" : libdirs
  },
  { "name"    : "gimg_tga",
    "type"    : "dynamicmodule",
    "ext"     : ".ipl",
    "prefix"  : "share/plugins/gimg",
    "srcs"    : ["src/plugins/tga/tga.cpp"],
    "libs"    : ["gimg", "gcore"],
    "incdirs" : ["include", gcore_inc],
    "libdirs" : libdirs
  },
  { "name"    : "gimg_hdr",
    "type"    : "dynamicmodule",
    "ext"     : ".ipl",
    "prefix"  : "share/plugins/gimg",
    "srcs"    : ["src/plugins/hdr/hdr.cpp"],
    "libs"    : ["gimg", "gcore"],
    "incdirs" : ["include", gcore_inc],
    "libdirs" : libdirs
  },
  { "name"    : "gimg_tests",
    "type"    : "testprograms",
    "srcs"    : glob.glob("src/tests/*.cpp"),
    "libs"    : ["gimg", "gcore"],
    "deps"    : ["gimg_bmp", "gimg_tga", "gimg_hdr"],
    "incdirs" : ["include", gcore_inc],
    "libdirs" : libdirs
  },
  { "name"    : "view",
    "type"    : "program",
    "srcs"    : glob.glob("src/bin/*.cpp") + ["src/bin/glew.c"],
    "libs"    : ["gimg", "gcore"],
    "defs"    : ["GLEW_STATIC", "GLEW_BUILD"],
    "custom"  : [glut.Require],
    "deps"    : ["gimg_bmp", "gimg_tga", "gimg_hdr"],
    "incdirs" : ["include", gcore_inc],
    "libdirs" : libdirs
  }
]

env = excons.MakeBaseEnv()
excons.DeclareTargets(env, prjs)




