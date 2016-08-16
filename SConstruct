import os
import glob
import excons
from excons.tools import threads
from excons.tools import dl

static = (excons.GetArgument("static", 0, int) != 0)

gcore_inc, gcore_lib = excons.GetDirs("gcore", silent=True)

if gcore_inc is None and gcore_lib is None:
  try:
    Import("RequireGcore")
  except:
    SConscript("gcore/SConstruct")
    Import("RequireGcore")
else:
  # Define our own RequireGcore
  def RequireGcore(*args, **kwargs):
    def _Require(env):
      if gcore_inc:
        env.Append(CPPPATH=[gcore_inc])
      if gcore_lib:
        env.Append(LIBPATH=[gcore_lib])
      env.Append(LIBS=["gcore"])
      if not str(Platform()) in ["win32", "darwin"]:
        env.Append(LIBS=["rt"])
      if static:
        env.Append(CPPDEFINES=["GCORE_STATIC"])
        threads.Require(env)
        dl.Require(env)
    
    return _Require


def RequireGnet(env):
  # Don't need to set CPPPATH, headers are now installed in output directory
  # Don't need to set LIBPATH, library output directory is automatically added by excons
  env.Append(LIBS=["gnet"])
  
  if static:
    env.Append(CPPDEFINES=["GNET_STATIC"])
  
  RequireGcore(env)

Export("RequireGnet")


prjs = [
  { "name"         : "gnet",
    "type"         : "staticlib" if static else "sharedlib",
    "version"      : "0.1.4",
    "soname"       : "libgnet.so.0",
    "install_name" : "libgnet.0.dylib",
    "srcs"         : glob.glob("src/lib/*.cpp"),
    "defs"         : ["GNET_STATIC"] if static else ["GNET_EXPORTS"] ,
    "install"      : {"include": ["include/gnet"]},
    "custom"       : [RequireGcore],
    "deps"         : ["gcore"]
  },
  { "name"    : "gnet_tests",
    "type"    : "testprograms",
    "srcs"    : glob.glob("src/tests/*.cpp"),
    "custom"  : [RequireGnet]
  }
]

env = excons.MakeBaseEnv()
excons.DeclareTargets(env, prjs)

Default(["gnet"])
