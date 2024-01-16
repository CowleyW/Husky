from subprocess import call
import os

if not os.path.isdir("lib/"):
    print("Directory lib/ does not exist. Creating it.")
    call(["mkdir", "lib"])
else:
    print("Directory lib/ exists. Continuing.")

# Set the current working directory to $ROOT/lib/
os.chdir("lib/")

if not os.path.isdir("Catch2/"):
    print("Dependency Catch2 does not exist. Cloning it.")
    call(["git", "clone", "git@github.com:catchorg/Catch2.git"])
else:
    print("Dependency Catch2 exists. Continuing.")

# Restore current working directory to project root
os.chdir("..")
