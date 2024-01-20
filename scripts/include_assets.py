import argparse
import os

def include_assets(assets_path):
    # Move to the assets folder
    old_path = os.getcwd()
    os.chdir(assets_path)

    full_files = []

    for (folder, _, files) in os.walk(assets_path):
        start = folder[len(assets_path):].replace("\\", "/")
        full_files.extend([start + "/" + f for f in files])

    print(full_files)
    # Restore the old path
    os.chdir(old_path)
