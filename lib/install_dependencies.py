from subprocess import call
import os

# Set the current working directory to $ROOT/lib/
os.chdir("lib/")

if not os.path.isdir("Catch2/"):
    call(["git", "clone", "git@github.com:catchorg/Catch2.git"])

# Restore current working directory to project root
os.chdir("..")
