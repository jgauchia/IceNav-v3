# CLI

IceNav CLI is reachable over Serial and, when enabled, over Telnet (port 11000); type `help` on the device to list the available commands. Below are some extra details about them.

**klist**: List user custom settings (example of custom settings):

```
    KEYNAME     DEFINED         VALUE
    =======     =======         =====
    defZoom     custom          17             Default zoom
    vectMap     custom          false          Vectorized map
   mapSpeed     custom          true           Show speed meter in map
   mapScale     custom          true           Show scale meter in map
    mapComp     custom          true           Show compass in map
 mapCompRot     custom          true           Rotate map with the compass
  showClimb     custom          true           Show Climb analyzer when following track
      map3D     custom          true           Show Pseudo 3D view when navigating track or waypoint
     simNav     custom          false          Indicates whether navigation simulation mode is enabled or disabled
      gpsTX     custom          43             GPS Tx gpio
      gpsRX     custom          44             GPS Rx gpio
     defLAT     custom          52.5200        Default latitude
     defLON     custom          13.4049        Default longitude
  defBright     custom          255            Default screen bright (0-255)
   VmaxBatt     custom          4.19999981     Battery max. voltage
   VminBatt     custom          3.59999990     Battery min. voltage
   tempOffs     custom          0              Temperature offset (-/+)
      defTZ     custom          Europe/Madrid  TZ identifier (see lib/utils/src/timezone.c default UTC)
  defDecAng     custom          0.22000000     Default declination angle
  kalmanFil     custom          true           Enable compass Kalman Filter
    kalmanQ     custom          0.00500000     Def. Kalman Filter const. Process noise covariance (0-1)
    kalmanR     custom          0.60000000     Def. Kalman Filter const. Measurement noise covariance (0-1)
 routeSpeed     custom          130            Max speed (km/h) for A* heuristic: 130=car, 25=bike, 5=walk. Selects ROUTE/CAR|BIKE|WALK/ROUTE.bin on SD card.
nmeaDbgTile     custom          false          Show the NMEA debug tile
 logProfile     custom          0              GPX logger activity profile index: 0 = Walk (default), 1 = Bike, 2 = Car.
```

**kset KEYNAME**: Set user custom settings:

In order to simplify the configuration of the device (minimum and maximum battery level, default position, etc...) via CLI it is possible to specify default values for the configuration.
This has been done in order to speed up the device configuration process without having to invest "time" in modifying and creating extra configuration screens in the GUI (LVGL).

Available user parameters can be obtained using the **klist** command with a CLI connection (either via USB connection or TELNET connection)

**nmcli**: IceNav use a `wcli` network manager library. For more details of this command and its sub commands please refer to [here](https://github.com/hpsaturn/esp32-wifi-cli?tab=readme-ov-file#readme)

**outnmea**: this command toggle the GPS output to the serial console. With that it will be compatible with external GPS software like `PyGPSClient` and others. To stop these messages in your console, just only repeat the same command or perform a `CTRL+C`.

**scshot**: This utility can save a PNG screenshot to the root of your SD, with the name: `screenshot.png`.

This screenshot command can send the screenshot over WiFi using the following syntax (replace IP with your PC IP):

```bash
scshot 192.168.1.10 8123
```

Ensure your PC has the specified port open and firewall access enabled to receive the screenshot via the `netcat` command, like this:

```bash
nc -l -p 8123 > screenshot.png
```

Additionally, you can download the screenshot with webfile server.
