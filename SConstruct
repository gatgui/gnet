import glob
import excons

SConscript("gcore/SConstruct")

static = int(ARGUMENTS.get("static", "0"))
liblibs = [] if static else ["gcore"]

prjs = [
  { "name"    : "gnet",
    "type"    : "staticlib" if static else "sharedlib",
    "srcs"    : glob.glob("src/lib/*.cpp"),
    "defs"    : ["GNET_STATIC", "GCORE_STATIC"] if static else ["GNET_EXPORTS"] ,
    "libs"    : liblibs,
    "incdirs" : ["gcore/include", "include"]
  },
  { "name"    : "gnet-tests",
    "type"    : "testprograms",
    "srcs"    : glob.glob("src/tests/*.cpp"),
    "defs"    : ["GNET_STATIC", "GCORE_STATIC"] if static else [],
    "libs"    : ["gnet", "gcore"],
    "incdirs" : ["gcore/include", "include"]
  }
]

env = excons.MakeBaseEnv()
excons.DeclareTargets(env, prjs)




