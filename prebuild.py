# CanAirIO Project
# Author: @hpsaturn
# pre-build script, setting up build environment

import os.path
from platformio import util
import shutil
from SCons.Script import DefaultEnvironment

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

# print ("environment:")
# print (env.Dump())

# get runtime credentials and put them to compiler directive
env.Append(BUILD_FLAGS=[
    u'-DREVISION=' + revision + '',
    u'-DVERSION=\\"' + version + '\\"',
    u'-DFLAVOR=\\"' + flavor + '\\"',
    u'-D'+ flavor + '=1'
    ])

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