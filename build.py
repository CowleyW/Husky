import argparse
import os
from subprocess import call
from scripts.include_assets import include_assets

def configure():
    call(["cmake", "-G", "MinGW Makefiles", "-S", ".", "-B", "out/"])
    call(["cp", "out/compile_commands.json", "compile_commands.json"])


def build_test(run):
    call(["cmake", "-G", "MinGW Makefiles", "-S", ".", "-B", "out/"])

    os.chdir("out")
    call(["make", "tests"])

    if (run):
        call(["tests.exe"])

    os.chdir("..")


def build_main(run):
    call(["cmake", "-G", "MinGW Makefiles", "-S", ".", "-B", "out/"])

    os.chdir("out")
    call(["make", "runtime"])

    if (run):
        call(["runtime.exe"])

    os.chdir("..")


# Build script main
if __name__ == "__main__":
    include_assets(os.getcwd() + "/assets/")
    parser = argparse.ArgumentParser(description="build")
    parser.add_argument("--test", dest="build", action="store_const",
                        const=build_test, default=build_main,
                        help="build tests")
    parser.add_argument("--configure", dest="configure", action="store_true", 
                        help="only configure CMake")
    parser.add_argument("--run", dest="run", action="store_true",
                        help="run the executable")

    args = parser.parse_args()

    if args.configure:
        configure()
    else:
        args.build(args.run)

