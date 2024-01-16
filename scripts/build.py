import argparse
import os
from subprocess import call

# Create bin/ directory then create Debug/ and Release/ subdirectories
def make_bin():
    if not os.path.isdir("bin/"):
        call(["mkdir", "bin"])
    if not os.path.isdir("bin/Debug/"):
        call(["mkdir", "bin/Debug"])
    if not os.path.isdir("bin/Release/"):
        call(["mkdir", "bin/Release"])


# Build application tests and move to bin/
def build_test():
    call(["cmake", "-S", ".", "-B", "out/"])

    os.chdir("out")
    call(["msbuild", "Draco.sln", "-target:Tests"])

    os.chdir("..")
    call(["cp", "out/Debug/Tests.exe", "bin/Debug/Tests.exe"])


# Build the main application and move it to bin/
def build_main():
    call(["cmake", "-S", ".", "-B", "out/"])

    os.chdir("out")
    call(["msbuild", "Draco.sln", "-target:Draco"])

    os.chdir("..")


    call(["cp", "out/Debug/Draco.exe", "bin/Debug/Draco.exe"])


# Build script main
if __name__ == "__main__":
    make_bin()
    
    parser = argparse.ArgumentParser(description="build")
    parser.add_argument("--test", dest="build", action="store_const",
                        const=build_test, default=build_main,
                        help="build tests")

    args = parser.parse_args()
    args.build()

