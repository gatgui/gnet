import glob
import excons

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
  { "name"  : "gnet",
    "type"  : "sharedlib",
    "srcs"  : glob.glob("src/lib/*.cpp"),
    "defs"  : ["GNET_EXPORTS"],
    "libs"  : ["gcore"],
    "incdirs": [gcore_inc],
    "libdirs": [gcore_lib]
  },
  { "name"  : "gnet-tests",
    "type"  : "testprograms",
    "srcs"  : glob.glob("src/tests/*.cpp"),
    "libs"  : ["gnet", "gcore"],
    "incdirs": [gcore_inc],
    "libdirs": [gcore_lib]
  }
]

env = excons.MakeBaseEnv()
excons.DeclareTargets(env, prjs)




