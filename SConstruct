import os
import glob
import excons

static = (excons.GetArgument("static", 0, int) != 0)

SConscript("gcore/SConstruct")
Import("RequireGcore")

def RequireGnet(subdir=None):
  if subdir and type(subdir) in (str, unicode):
    if not (subdir.endswith("/") or subdir.endswith("\\")):
      subdir += "/"
  else:
    subdir = ""

  def _Require(env):
    env.Append(CPPPATH=[subdir+"include"])
    # Don't need to set LIBPATH, library output directory is automatically added by excons
    env.Append(LIBS=["gnet"])
    
    if static:
      env.Append(CPPDEFINES=["GNET_STATIC"])
    
    RequireGcore(subdir=subdir+"gcore")(env)

  return _Require

Export("RequireGnet")


prjs = [
  { "name"         : "gnet",
    "type"         : "staticlib" if static else "sharedlib",
    "version"      : "0.1.3",
    "soname"       : "libgnet.so.0",
    "install_name" : "libgnet.0.dylib",
    "srcs"         : glob.glob("src/lib/*.cpp"),
    "defs"         : ["GNET_STATIC"] if static else ["GNET_EXPORTS"] ,
    "incdirs"      : ["include"],
    "custom"       : [RequireGcore(subdir="gcore")]
  },
  { "name"    : "gnet_tests",
    "type"    : "testprograms",
    "srcs"    : glob.glob("src/tests/*.cpp"),
    "custom"  : [RequireGnet()]
  }
]

env = excons.MakeBaseEnv()
excons.DeclareTargets(env, prjs)
