import os
import glob

initsubs = False

static = (int(ARGUMENTS.get("static", "0")) != 0)

try:
  import excons
  if len(glob.glob("gcore/*")) == 0:
    # No files in gcore directory
    prefix = ARGUMENTS.get("with-gcore", None)
    inc = ARGUMENTS.get("with-gcore-inc", None)
    if (static and (inc or prefix)) or (not static and prefix):
      # gcore externally provided
      pass
    else:
      initsubs = True
except:
    initsubs = True

if initsubs:
  import subprocess
  subprocess.Popen("git submodule init", shell=True).communicate()
  subprocess.Popen("git submodule update", shell=True).communicate()
  
  import excons

liblibs = [] if static else ["gcore"]
libdirs = []

gcore_base = ARGUMENTS.get("with-gcore", None)
gcore_inc = None
gcore_lib = None
if gcore_base is None:
  gcore_inc = ARGUMENTS.get("with-gcore-inc", None)
  gcore_lib = ARGUMENTS.get("with-gcore-lib", None)
  if gcore_inc is None and gcore_lib is None:
    gcore_inc = "gcore/include"
    SConscript("gcore/SConstruct")
  elif gcore_lib != None:
    libdirs.append(gcore_lib)
else:
  gcore_inc = os.path.join(gcore_base, "include")
  gcore_lib = os.path.join(gcore_base, "lib")
  libdirs.append(gcore_lib)

prjs = [
  { "name"    : "gnet",
    "type"    : "staticlib" if static else "sharedlib",
    "srcs"    : glob.glob("src/lib/*.cpp"),
    "defs"    : ["GNET_STATIC", "GCORE_STATIC"] if static else ["GNET_EXPORTS"] ,
    "libs"    : liblibs,
    "incdirs" : [gcore_inc, "include"],
    "libdirs" : libdirs
  },
  { "name"    : "gnet_tests",
    "type"    : "testprograms",
    "srcs"    : glob.glob("src/tests/*.cpp"),
    "defs"    : ["GNET_STATIC", "GCORE_STATIC"] if static else [],
    "libs"    : ["gnet", "gcore"],
    "incdirs" : [gcore_inc, "include"],
    "libdirs" : libdirs
  }
]

env = excons.MakeBaseEnv()
excons.DeclareTargets(env, prjs)




