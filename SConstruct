import glob
import excons

SConscript("gcore/SConstruct")

prjs = [
  { "name"    : "gnet",
    "type"    : "sharedlib",
    "srcs"    : glob.glob("src/lib/*.cpp"),
    "defs"    : ["GNET_EXPORTS"],
    "libs"    : ["gcore"],
    "incdirs" : ["gcore/include", "include"]
  },
  { "name"    : "gnet-tests",
    "type"    : "testprograms",
    "srcs"    : glob.glob("src/tests/*.cpp"),
    "libs"    : ["gnet", "gcore"],
    "incdirs" : ["gcore/include", "include"]
  }
]

env = excons.MakeBaseEnv()
excons.DeclareTargets(env, prjs)




