import argparse
import os
from subprocess import call
from scripts.include_assets import include_assets

# Write to run_flag
def set_run_flag(text):
    with open("run_flag", "w") as f:
        f.write(text)


# Build using MSVC compiler
def build_msvc():
    call(["cmake", "-G", "Visual Studio 17 2022", "-S", ".", "-B", "out/msvc"])

    os.chdir("out/msvc")
    call(["msbuild", "ALL_BUILD.vcxproj"])

    os.chdir("..")
    set_run_flag("msvc")
    os.chdir("..")


# Build using GNU compiler
def build_gnu():
    call(["cmake", "-G", "MinGW Makefiles", "-S", ".", "-B", "out/gnu"])

    os.chdir("out/gnu")
    call(["make", "runtime"])
    call(["make", "test"])

    os.chdir("..")
    set_run_flag("gnu")
    os.chdir("..")


# Build script main
if __name__ == "__main__":
    include_assets(os.getcwd() + "/assets/")
    parser = argparse.ArgumentParser(description="build")
    parser.add_argument("--gnu", dest="gnu", action="store_true", 
                        help="Build using the GNU compiler")

    args = parser.parse_args()
    if args.gnu:
        build_gnu()
    else:
        build_msvc()

