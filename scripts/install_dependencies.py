from subprocess import call
import os

def create_lib_folder():
    if not os.path.isdir("lib/"):
        print("Directory lib/ does not exist. Creating it.")
        call(["mkdir", "lib"])
    else:
        print("Directory lib/ exists. Continuing.")


def install(name, repo):
    os.chdir("lib/")
    
    if not os.path.isdir(name):
        print(f"Dependency {name} does not exist. Cloning it.")
        call(["git", "clone", repo])
    else:
        print(f"Dependency {name} exists. Continuing.")
    
    os.chdir("..")

if __name__ == "__main__":
    create_lib_folder()

    install("Catch2", "git@github.com:catchorg/Catch2.git")
    install("glfw", "git@github.com:glfw/glfw.git")

