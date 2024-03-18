import argparse
import os
import shutil
from subprocess import call

def git_clone(repo_url, repo_name, clean):
    path = f"lib/{repo_name}"

    if clean and os.path.isdir(path):
        print(f"Cleaning {path}")
        shutil.rmtree(path)

    if not os.path.isdir(path):
        print(f"{path} does not exist, installing it.")
        call(["git", "clone", repo_url, path])
    else:
        print(f"{path} already exists, ignoring it.")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="install dependencies")
    parser.add_argument("--clean", dest="clean", action="store_true",
                        help="clean the libs directory before installing")
    args = parser.parse_args()

    git_clone("git@github.com:catchorg/Catch2.git", "Catch2", args.clean)
    git_clone("git@github.com:glfw/glfw.git", "glfw", args.clean)
    git_clone("git@github.com:g-truc/glm.git", "glm", args.clean)
    git_clone("git@github.com:fmtlib/fmt.git", "fmt", args.clean)
    git_clone("git@github.com:chriskohlhoff/asio.git", "asio", args.clean)
    git_clone("git@github.com:charles-lunarg/vk-bootstrap.git", "vk-bootstrap", args.clean)
    git_clone("git@github.com:GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git", "VulkanMemoryAllocator", args.clean)
    git_clone("git@github.com:KhronosGroup/Vulkan-Utility-Libraries.git", "Vulkan-Utility-Libraries", args.clean)

