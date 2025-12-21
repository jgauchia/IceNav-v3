"""
Pre-build script for IceNav ESP-IDF migration
Removes ARM-specific LVGL files that are incompatible with Xtensa (ESP32)
"""

Import("env")
import os
import shutil

def remove_arm_files(source, target, env):
    """Remove ARM/Helium/Neon assembly files from LVGL library"""

    libdeps_dir = os.path.join(env.subst("$PROJECT_LIBDEPS_DIR"), env.subst("$PIOENV"))
    lvgl_blend_path = os.path.join(libdeps_dir, "lvgl", "src", "draw", "sw", "blend")

    # Folders containing ARM-specific assembly that breaks Xtensa builds
    arm_folders = ["helium", "arm2d", "neon"]

    removed = []
    for folder in arm_folders:
        folder_path = os.path.join(lvgl_blend_path, folder)
        if os.path.exists(folder_path):
            try:
                shutil.rmtree(folder_path)
                removed.append(folder)
            except Exception as e:
                print(f"Warning: Could not remove {folder}: {e}")

    if removed:
        print(f"Pre-build: Removed LVGL ARM folders: {', '.join(removed)}")

# Register the pre-build action
env.AddPreAction("buildprog", remove_arm_files)

# Also run at script load time for initial builds
libdeps_dir = os.path.join(env.subst("$PROJECT_LIBDEPS_DIR"), env.subst("$PIOENV"))
lvgl_blend_path = os.path.join(libdeps_dir, "lvgl", "src", "draw", "sw", "blend")

arm_folders = ["helium", "arm2d", "neon"]
for folder in arm_folders:
    folder_path = os.path.join(lvgl_blend_path, folder)
    if os.path.exists(folder_path):
        try:
            shutil.rmtree(folder_path)
            print(f"Pre-build: Removed {folder_path}")
        except:
            pass
