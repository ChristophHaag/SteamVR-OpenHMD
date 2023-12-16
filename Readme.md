# SteamVR Plugin for OpenHMD Drivers

# Status/TODO

* controller bindings are WIP. Oculus CV1 Touch Controllers should work, with others YMMV.
* controller models - OpenHMD doesn't provide a method ([yet](https://github.com/OpenHMD/OpenHMD/issues/119)) to get rendermodels.
* Device selection is very basic. To change the default you can use the config file mechanism (on Linux) or change the default config in ohmd_config.h in the else branch (index 2 and 3 are controllers) before compiling

Possible improvements:

* updating tracking data asynchronously in a separate thread might improve smoothness (if there is a problem)
* feeding vector acceleration values to SteamVR? Would that improve tracking or make OpenHMD's tracking worse?


## Before building: Use latest git OpenHMD (optional)

    cd subprojects/openhmd
    git pull origin master

## Install needed packages (Ubuntu):

    sudo apt-get install build-essential cmake libhidapi-dev 

## Build with cmake:

    git clone --recursive https://github.com/ChristophHaag/SteamVR-OpenHMD.git
    cd SteamVR-OpenHMD
    mkdir build
    cd build
    cmake ..
    make

OpenHMD is included as a git submodule. An OpenHMD shared library will be built first and the steamvr plugin will link to the OpenHMD library built in `build/external/openhmd/libopenhmd.so`. If you want to package the SteamVR plugin, make sure you have libopenhmd.so in your library search path or package the openhmd library too and change the rpath.

If you use the Vive and want to use the (imperfect) values from https://github.com/OpenHMD/OpenHMD/pull/90, go to external/openhmd and run `git pull origin pull/90/head` before the make step to merge the pull request locally.

# Install udev rules

OpenHMD requires direct access to the USB interface of VR HMDs.

It's recommended to install xr-hardware, a well maintained collection of udev rules.

https://gitlab.freedesktop.org/monado/utilities/xr-hardware

## Run (easy for linux):

Run `./register.sh`.
add
This overwrites SteamVR's settings with a steamvr.vrsettings that disables all SteamVR hardware plugins that ship with SteamVR (Vive lighthouse, Oculus, etc). The current SteamVR config will be backed up to steamvr-config-backup/. Then it registers the current build directory as a SteamVR plugin.

To do the reverse, run `./unregister.sh`.

## Run (detailed for linux, adapt to your operating system)

First register the driver with SteamVR:

    ~/.local/share/Steam/steamapps/common/SteamVR/bin/linux64/vrpathreg adddriver ~/SteamVR-OpenHMD/build

The directory given to vrpathreg should contain `driver.vrdrivermanifest`, `resources/` and `bin/linux64/driver_openhmd.so`.

If you use a HMD for which SteamVR already has a plugin (currently Vive and Oculus Rift), copy the steamvr.vrsettings file that disables those plugins into Steam's config directory.

    cp ~/SteamVR-OpenHMD/steamvr.vrsettings ~/.local/share/Steam/config/steamvr.vrsettings

Don't forget to make a backup if you have special SteamVR settings.

Now run SteamVR and check ~/.local/share/Steam/logs/vrserver.txt for errors.

## Remove (detailed)

Lets remove the driver from the vrpathregistry.
First we will check that the driver is indeed registered and where the registry points to:

    LD_LIBRARY_PATH=~/.steam/steam/steamapps/common/SteamVR/bin/linux64 ~/.steam/steam/steamapps/common/SteamVR/bin/linux64/vrpathreg
    
Adapt the command if Steam or SteamVR are installed in a different path.
This command will output something like the following:

    Runtime path = /home/user/.steam/steam/steamapps/common/SteamVR
    Config path = /home/user/.steam/steam/config
    Log path = /home/user/.steam/steam/logs
    External Drivers:
    	/home/user/Documents/SteamVR-OpenHMD/build

Now we can remove the driver from the registry by giving `removedriver /path/`to vrpathreg, while replacing the path with the one listed in "External Drivers:" like so:

    LD_LIBRARY_PATH=~/.steam/steam/steamapps/common/SteamVR/bin/linux64 ~/.steam/steam/steamapps/common/SteamVR/bin/linux64/vrpathreg removedriver  /home/user/Documents/SteamVR-OpenHMD/build

Now you can safely remove SteamVR-OpenHMD and OpenHMD.

## Build and run with docker:

    git clone --recursive https://github.com/ChristophHaag/SteamVR-OpenHMD.git
    cd SteamVR-OpenHMD
    docker.sh

This will create a docker container running the same GCC as Steam uses, so the driver will be compatible with Steam runtime libstdc++ library.
The script docker.sh will create the container, build the driver, register it with steam (using `vrpathreg adddriver` as described above) and, if the build is successful, it launches steamVR using STEAM_RUNTIME_PREFER_HOST_LIBRARIES=0 so Steam uses it's own runtime environment to run steamVR.

This method is simpler to build the driver and builds a driver fully compatible with the steam runtime, no matter the distro you're running. You just need to have Docker installed!



## Configuration:

### Display refresh rate

To tell SteamVR to use a speciifc display refresh rate for the HMD, consider editing `displayFrequency` in SteamVR-OpenHMD/build/resources/settings/default.vrsettings.

### OpenHMD devices

Upstream pull request to follow: https://github.com/OpenHMD/OpenHMD/issues/8

For now. $HOME/.ohmd_config.txt is used.

Example content for a HMD 0, nolo HMD tracker 1, nolo controllers 2 and 3:

    hmddisplay 0
    hmdtracker 1
    leftcontroller 2
    rightcontroller 3

or a single HMD 0 with no controllers

    hmddisplay 0
    hmdtracker 0
    leftcontroller -1
    rightcontroller -1

This defines 4 openhmd devices.

* hmdddisplay is opened for the display config. Choose this for the actual HMD like Vive, Oculus Rift, etc.
* hmdtracker is opened for tracking the head. Choose a different index than the HMD if you have a NOLO tracker (or in the future a Vive tracker).
* leftcontroller and rightcontroller are the indices for the controllers. There are no separate trackers for controllers for now but it's easy to hack in.

If the config file is not available (probably only works on linux), default values are used. Change them in ohmd_config.h.

There is also the option `autodetect 1` which enables autodetection (enabled if you have no config file, and normally disabled if you have a config file). Disabling autodetection is important because you may or may not wish the OpenHMD "null" controllers to be present.

### Gaze Pointer with gamepad

At least SteamVR Home supports controller based navigation, however it is only enabled when the Manufacturer string provided by the plugin is "Oculus".

The Manufacturer is usually set to what OpenHMD provides as hardware vendor but in SteamVR-OpenHMD the manufacturer string can be overriden with an environment variable.

Start SteamVR like this:

    OHMD_VENDOR_OVERRIDE=Oculus ~/.steam/steam/SteamApps/common/SteamVR/bin/vrstartup.sh

Alternatively in the Steam GUI set the SteamVR launch options to

    OHMD_VENDOR_OVERRIDE=Oculus %command%

You can verify that the environment variable is set with `grep "driver_openhmd: Vendor:" ~/.steam/steam/logs/vrserver.txt`.

## Creating additional Udev Rules:

The VR hardware in this section is already supported by xr-hardware. If you want to create your own udev rules, here is how.

To allow OpenHMD to access any devices, udev rules need to be set on most linux systems.
Add a file named `83-hmd.rules` to `/etc/udev/rules.d/`, or your distributions equivalent.

As an example the content of this file could look like this:

    #Oculus DK1 DK2 CV1
    SUBSYSTEM=="usb", ATTR{idVendor}=="2833", MODE="0666", GROUP="plugdev"
    #Vive
    SUBSYSTEM=="usb", ATTR{idVendor}=="0bb4", MODE="0666", GROUP="plugdev"
    #Deepoon
    SUBSYSTEM=="usb", ATTR{idVendor}=="0483", MODE="0666", GROUP="plugdev"
    #Sony PSVR
    SUBSYSTEM=="usb", ATTR{idVendor}=="054c", MODE="0666", GROUP="plugdev"
    #OSVR
    SUBSYSTEM=="usb", ATTR{idVendor}=="1532", MODE="0666", GROUP="plugdev"
    #Pimax 4K
    SUBSYSTEM=="usb", ATTR{idVendor}=="2833", MODE="0666", GROUP="plugdev"
    #NOLO CV1
    SUBSYSTEM=="usb", ATTR{idVendor}=="0483", MODE="0666", GROUP="plugdev"
    #Samsung GearVR
    SUBSYSTEM=="usb", ATTR{idVendor}=="04E8", MODE="0666", GROUP="plugdev"
    #HP Mixed Reality
    SUBSYSTEM=="usb", ATTR{idVendor}=="03F0", MODE="0666", GROUP="plugdev"
    #Lenovo Mixed Reality
    SUBSYSTEM=="usb", ATTR{idVendor}=="045e", MODE="0666", GROUP="plugdev"
    #DreamWorld DreamGlass AR
    SUBSYSTEM=="usb", ATTR{idVendor}=="0483", MODE="0666", GROUP="plugdev"


## License

SteamVR-OpenHMD is released under the permissive Boost Software License (see LICENSE for more information), to make sure it can be linked and distributed with both free and non-free software. While it doesn't require contribution from the users, it is still very appreciated.

