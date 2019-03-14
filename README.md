# SteamVR-OpenHMD
A SteamVR plugin for OpenHMD drivers.

## Status / TODO
* controller buttons are currently unimplemented and pressing any button does nothing
* controller models - OpenHMD doesn't provide a method ([yet](https://github.com/OpenHMD/OpenHMD/issues/119)) to get rendermodels, so default (Steam Gamepad controller) is used
* Controllers are not used by default because of those limitations. To change the default you can use the config file mechanism (on Linux) or change the default config in ohmd_config.h in the else branch (index 2 and 3 are controllers) before compiling
### Possible improvements
* updating tracking data asynchronously in a separate thread might improve smoothness (if there is a problem)
* feeding vector acceleration values to SteamVR? Would that improve tracking or make OpenHMD's tracking worse?

## Install dependencies
- Install the following packages (Ubuntu, adjust this for your currently installed distribution)
```sh
sudo apt-get install build-essential cmake libhidapi-dev git
```
- Install [OpenHMD](https://github.com/OpenHMD/OpenHMD).
- Install [openvr](https://github.com/ValveSoftware/openvr).

## Build using CMake
```sh
git clone https://github.com/ChristophHaag/SteamVR-OpenHMD.git
cd SteamVR-OpenHMD
mkdir build
cd build
cmake ..
make
```

If you use the Vive and want to use the (imperfect) values from https://github.com/OpenHMD/OpenHMD/pull/90, clone OpenHMD,  execute `git pull origin pull/90/head` before the make step and continue installing.

## Run (easier way)
```sh
./register.sh
```

This overwrites SteamVR's settings with a `steamvr.vrsettings` that disables all SteamVR hardware plugins that ship with SteamVR, examples being VIVE Lighthouse and Oculus. The current SteamVR configuration will be backed up to `steamvr-config-backup/`, then it registers the current build directory as a SteamVR plugin.
To do the reverse, execute the following

```sh
./unregister.sh
```

## Run (detailed way, adapt to your operating system)
First register the driver with SteamVR:

```sh
~/.local/share/Steam/steamapps/common/SteamVR/bin/linux64/vrpathreg adddriver ~/SteamVR-OpenHMD/build
```

The directory given to vrpathreg should contain `driver.vrdrivermanifest`, `resources/` and `bin/linux64/driver_openhmd.so`.
If you use a HMD for which SteamVR already has a plugin (currently Vive and Oculus Rift), copy the `steamvr.vrsettings` file that disables those plugins into Steam's config directory.

```sh
cp ~/SteamVR-OpenHMD/steamvr.vrsettings ~/.local/share/Steam/config/steamvr.vrsettings
```

Don't forget to make a backup if you have customized your SteamVR settings.
Now run SteamVR and check `~/.local/share/Steam/logs/vrserver.txt` for errors.

## Build and run with Docker
```sh
git clone https://github.com/ChristophHaag/SteamVR-OpenHMD.git
cd SteamVR-OpenHMD
docker.sh
```

This will create a docker container running the same GCC as Steam uses, so the driver will be compatible with Steam's `libstdc++` runtime library.
The script `docker.sh` will create the container, build the driver, register it with Steam (using `vrpathreg adddriver` as described above) and, if the build is successful, launches SteamVR using `STEAM_RUNTIME_PREFER_HOST_LIBRARIES=0`, so Steam uses it's own runtime environment to run SteamVR.

This method is simpler to build the driver, and builds a driver fully compatible with the Steam runtime, no matter the distro you're running. You just need to have Docker installed.

## Configuration
[Upstream pull request to follow](https://github.com/OpenHMD/OpenHMD/issues/8).
For now, `$HOME/.ohmd_config.txt` is used.

Example content for a HMD 0, nolo HMD tracker 1, nolo controllers 2 and 3

```
hmddisplay 0
hmdtracker 1
leftcontroller 2
rightcontroller 3
```

or a single HMD 0 with no controllers

```
hmddisplay 0
hmdtracker 0
leftcontroller -1
rightcontroller -1
```

This defines 4 OpenHMD devices.

* `hmdddisplay` is opened for the display config. Choose this for the actual HMD like Vive, Oculus Rift, etc.
* `hmdtracker` is opened for tracking the head. Choose a different index than the HMD if you have a NOLO tracker (or in the future a Vive tracker).
* `leftcontroller` and ``rightcontroller` are the indices for the controllers. There are no separate trackers for controllers for now but it's easy to hack in.

If the config file is not available (probably only works on linux), default values are used. Change them in ohmd_config.h.

### Gaze Pointer with gamepad
At least SteamVR Home supports controller based navigation, however it is only enabled when the Manufacturer string provided by the plugin is "Oculus".

The Manufacturer is usually set to what OpenHMD provides as hardware vendor, but in SteamVR-OpenHMD, the Manufacturer string can be overriden with an environment variable.

Start SteamVR like this

```sh
OHMD_VENDOR_OVERRIDE=Oculus ~/.steam/steam/SteamApps/common/SteamVR/bin/vrstartup.sh
```

Alternatively in the Steam GUI, set the SteamVR launch options to

```sh
OHMD_VENDOR_OVERRIDE=Oculus %command%
```

You can verify that the environment variable is set with `grep "driver_openhmd: Vendor:" ~/.steam/steam/logs/vrserver.txt`.

## udev Rules:
To allow OpenHMD to access any devices, udev rules need to be set on most Linux systems.
Add a file named `83-hmd.rules` to `/etc/udev/rules.d/`, or your distributions equivalent.

As an example the content of this file could look like the following

```
#Oculus DK1 DK2 CV1
SUBSYSTEM=="usb", ATTR{idVendor}=="2833", MODE="0666", GROUP="plugdev"'
#Vive
SUBSYSTEM=="usb", ATTR{idVendor}=="0bb4", MODE="0666", GROUP="plugdev"'
#Deepoon
SUBSYSTEM=="usb", ATTR{idVendor}=="0483", MODE="0666", GROUP="plugdev"'
#Sony PSVR
SUBSYSTEM=="usb", ATTR{idVendor}=="054c", MODE="0666", GROUP="plugdev"'
#OSVR
SUBSYSTEM=="usb", ATTR{idVendor}=="1532", MODE="0666", GROUP="plugdev"'
#Pimax 4K
SUBSYSTEM=="usb", ATTR{idVendor}=="2833", MODE="0666", GROUP="plugdev"'
#NOLO CV1
SUBSYSTEM=="usb", ATTR{idVendor}=="0483", MODE="0666", GROUP="plugdev"'
#Samsung GearVR
SUBSYSTEM=="usb", ATTR{idVendor}=="04E8", MODE="0666", GROUP="plugdev"'
#HP Mixed Reality
SUBSYSTEM=="usb", ATTR{idVendor}=="03F0", MODE="0666", GROUP="plugdev"'
#Lenovo Mixed Reality
SUBSYSTEM=="usb", ATTR{idVendor}=="045e", MODE="0666", GROUP="plugdev"'
#DreamWorld DreamGlass AR
SUBSYSTEM=="usb", ATTR{idVendor}=="0483", MODE="0666", GROUP="plugdev"'
```

## License
SteamVR-OpenHMD is released under the permissive Boost Software License (see LICENSE for more information), to make sure it can be linked and distributed with both free and non-free software. While it doesn't require contribution from the users, it is still very appreciated.
