# SteamVR Plugin for OpenHMD Drivers

## Build with cmake:

    git clone --recursive https://github.com/ChristophHaag/SteamVR-OpenHMD.git
    cd SteamVR-OpenHMD
    mkdir build
    cd build
    cmake ..
    make

OpenHMD is included as a git submodule. An OpenHMD shared library will be built first and the steamvr plugin will link to the OpenHMD library built in `build/external/openhmd/libopenhmd.so`. If you want to package the SteamVR plugin, make sure you have libopenhmd.so in your library search path or package the openhmd library too and change the rpath.

If you use the Vive and want to use the (imperfect) values from https://github.com/OpenHMD/OpenHMD/pull/90, go to external/openhmd and run `git pull origin pull/90/head` before the make step to merge the pull request locally.

## Run:

First register the driver with SteamVR:

    ~/.local/share/Steam/SteamApps/common/SteamVR/bin/linux64/vrpathreg adddriver ~/SteamVR-OpenHMD/build

The directory given to vrpathreg should contain `driver.vrdrivermanifest`, `resources/` and `bin/linux64/driver_openhmd.so`.

If you use a HMD for which SteamVR already has a plugin (currently Vive and Oculus Rift), copy the steamvr.vrsettings file that disables those plugins into Steam's config directory.

    cp ~/SteamVR-OpenHMD/steamvr.vrsettings ~/.local/share/Steam/config/steamvr.vrsettings

Don't forget to make a backup if you have special SteamVR settings.

Now run SteamVR and check ~/.local/share/Steam/logs/vrserver.txt for errors.

## Configuration:

Upstream pull request to follow: https://github.com/OpenHMD/OpenHMD/issues/8

For now. $HOME/.ohmd_config.txt is used.

Example content:

    hmddisplay 0
    hmdtracker 0
    leftcontroller 2
    rightcontroller 3

This defines 4 openhmd devices.
hmdddisplay is opened for the display config. Choose this for the actual HMD like Vive, Oculus Rift, etc.
hmdtracker is opened for tracking the head. Choose a different index than the HMD if you have a NOLO tracker (or in the future a Vive tracker).
leftcontroller and rightcontroller are the indices for the controllers. There are no separate trackers for controllers for now but it's easy to hack in.

If the config file is not available (probably only works on linux), default values are used. Change them in ohmd_config.h.
