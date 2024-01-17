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


def configure():
    call(["cmake", "-G", "MinGW Makefiles", "-S", ".", "-B", "out/"])
    call(["cp", "out/compile_commands.json", "compile_commands.json"])

# Build application tests and move to bin/
def build_test():
    call(["cmake", "-G", "MinGW Makefiles", "-S", ".", "-B", "out/"])

    os.chdir("out")
    call(["make", "triton_tests"])

    os.chdir("..")


# Build the main application and move it to bin/
def build_main():
    call(["cmake", "-G", "MinGW Makefiles", "-S", ".", "-B", "out/"])

    os.chdir("out")
    call(["make", "runtime"])

    os.chdir("..")


# Build script main
if __name__ == "__main__":
    make_bin()
    
    parser = argparse.ArgumentParser(description="build")
    parser.add_argument("--test", dest="build", action="store_const",
                        const=build_test, default=build_main,
                        help="build tests")
    parser.add_argument("--configure", dest="configure", action="store_true", 
                        help="only configure CMake")

    args = parser.parse_args()

    if args.configure:
        configure()
    else:
        args.build()

