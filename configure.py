from subprocess import call
from scripts.include_assets import include_assets

def configure():
    call(["cmake", "-G", "MinGW Makefiles", "-S", ".", "-B", "out/"])
    call(["cp", "out/compile_commands.json", "compile_commands.json"])

if __name__ == "__main__":
    configure()
