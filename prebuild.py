# CanAirIO Project
# Author: @hpsaturn
# pre-build script, setting up build environment

import json
import os.path
from platformio import util
import shutil
from SCons.Script import DefaultEnvironment, Exit
from platformio.dependencies import get_core_dependencies
from platformio.project.config import ProjectConfig

try:
    import configparser
except ImportError:
    import ConfigParser as configparser

# get platformio environment variables
env = DefaultEnvironment()
config = configparser.ConfigParser()
config.read("platformio.ini")

# get platformio source path
srcdir = env.get("PROJECTSRC_DIR")
flavor = env.get("PIOENV")
revision = config.get("common","revision")
version = config.get("common", "version")

dfl_lat = os.environ.get('ICENAV3_LAT')
dfl_lon = os.environ.get('ICENAV3_LON')

def readJson(path):
    try:
        with open(path) as handle:
            return json.load(handle)
    except (IOError, OSError, ValueError):
        return None

def coreDir():
    return ProjectConfig.get_instance().get("platformio", "core_dir")

# The platform and the core own the same tool-scons directory, so only one of the two versions
# can stay installed. On a mismatch the platform deletes that directory and the link then dies
# with "No module named 'SCons.Tool.FortranCommon'", see issue #390.
def requiredToolScons():
    platform = config.get("common", "platform")
    if "/download/" not in platform:
        return None
    tag = platform.rsplit("/download/", 1)[-1].split("/")[0]
    base = tag.split("-")[0]
    platformsDir = os.path.join(coreDir(), "platforms")
    try:
        candidates = sorted(os.listdir(platformsDir))
    except OSError:
        return None
    for candidate in candidates:
        manifest = readJson(os.path.join(platformsDir, candidate, "platform.json"))
        if not manifest:
            continue
        # a release tag may carry a rebuild suffix the manifest does not, 55.03.312-1 vs 55.03.312
        version = str(manifest.get("version", ""))
        if version in (tag, base) or version.startswith(base + "+"):
            return manifest.get("packages", {}).get("tool-scons", {}).get("package-version")
    return None

def installedToolScons():
    manifest = readJson(os.path.join(coreDir(), "packages", "tool-scons", "package.json"))
    return manifest.get("version") if manifest else None

required = requiredToolScons()
installed = installedToolScons()
if required and installed and required != installed:
    print("ERROR: PlatformIO toolchain mismatch, this build cannot be trusted.")
    print("  platform " + config.get("common", "platform") + " requires tool-scons " + required)
    print("  installed tool-scons: " + installed)
    print("  core asks for: " + str(get_core_dependencies().get("tool-scons")))
    print("  The platform deletes its SCons on a mismatch and the link then fails with")
    print("  \"No module named 'SCons.Tool.FortranCommon'\".")
    # Pairs below must follow the platform pin and the tool-scons pin of the CI workflows.
    print("  Working combinations:")
    print("    platform <= 55.03.311 with tool-scons 4.40801.0 (PlatformIO 6.1.x, pioarduino 6.1.19)")
    print("    platform >= 55.03.312 with tool-scons 4.41101.0 (PlatformIO 6.2.0+, pioarduino 6.2.0)")
    Exit(1)

# print ("environment:")
# print (env.Dump())

# get runtime credentials and put them to compiler directive
env.Append(BUILD_FLAGS=[
    u'-DREVISION=' + revision + '',
    u'-DVERSION=\\"' + version + '\\"',
    u'-DFLAVOR=\\"' + flavor + '\\"',
    u'-D'+ flavor + '=1'
    ])

# On ESP32-P4 an Espressif header (periph_ctrl.h) triggers -Wliteral-suffix.
# The suppression flag is C++ only, so it is added to CXXFLAGS (not BUILD_FLAGS)
# to avoid the "valid for C++ but not for C" warning on .c files.
if flavor.startswith("WAVESHARE_P4"):
    env.Append(CXXFLAGS=[u'-Wno-literal-suffix'])

# LVGL 9's style selector idiom (LV_PART_x | LV_STATE_y) mixes two distinct
# enum types by design; deprecated in C++20 but not a bug in our code.
env.Append(CXXFLAGS=[u'-Wno-deprecated-enum-enum-conversion'])

# NeoGPS fork (lib_deps) compares two distinct nmea/ubx enum types internally.
env.Append(CXXFLAGS=[u'-Wno-enum-compare'])

# Shellminator (lib_deps) calls the deprecated NetworkServer::available()
# (renamed to accept() in Arduino core 3.x).
env.Append(CXXFLAGS=[u'-Wno-deprecated-declarations'])

if dfl_lat != None and dfl_lon != None:
    print ("default lat: "+dfl_lat)
    print ("default lon: "+dfl_lon)
    env.Append(BUILD_FLAGS=[
        u'-DDEFAULT_LAT=' + dfl_lat + '',
        u'-DDEFAULT_LON=' + dfl_lon + ''
        ])

# NeoGps Config files
config_path = "lib/gps/GPSfix_cfg.h"
output_path =  ".pio/libdeps/" + flavor + "/NeoGPS/src" 
target_path = output_path + "/GPSfix_cfg.h"
os.makedirs(output_path, 0o755, True)
shutil.copy(config_path , target_path)

config_path = "lib/gps/NeoGPS_cfg.h"
output_path =  ".pio/libdeps/" + flavor + "/NeoGPS/src" 
target_path = output_path + "/NeoGPS_cfg.h"
os.makedirs(output_path, 0o755, True)
shutil.copy(config_path , target_path)

config_path = "lib/gps/NMEAGPS_cfg.h"
output_path =  ".pio/libdeps/" + flavor + "/NeoGPS/src" 
target_path = output_path + "/NMEAGPS_cfg.h"
os.makedirs(output_path, 0o755, True)
shutil.copy(config_path , target_path)