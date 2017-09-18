SteamVR Plugin for OpenHMD Devices

Build:
Run `cmake` and `make` for the OpenHMD base directory. The new OPENHMD_STEAMVR option is enabled by default. This will first build OpenHMD, and then the SteamVR plugin.

Run:
First register the driver with SteamVR:

    ~/.local/share/Steam/SteamApps/common/SteamVR/bin/linux64/vrpathreg adddriver ~/OpenHMD

or

    ~/.local/share/Steam/SteamApps/common/SteamVR/bin/linux64/vrpathreg adddriver ~/OpenHMD/build

if you build in a separate build directory.

The directory given to vrpathreg should contain `driver.vrdrivermanifest` and `bin/linux64/driver_openhmd.so` as a subdirectory.

Then copy the steamvr.vrsettings file that disables the lighthouse and oculus default driver into Steam's config directory.

    cp ~/OpenHMD/steamvr/steamvr.vrsettings ~/.local/share/Steam/config/steamvr.vrsettings

Don't forget to make a backup if you have special SteamVR settings. Now run SteamVR and check ~/.local/share/Steam/logs/vrserver.txt for errors.

TODO:
* implement controllers when openhmd controller api https://github.com/OpenHMD/OpenHMD/pull/93 is merged
* Perspective/eye separation. Possibly only GetProjectionRaw needs to be fixed: https://github.com/ValveSoftware/openvr/wiki/IVRSystem::GetProjectionRaw. SteamVR-OSVR does https://github.com/OSVR/SteamVR-OSVR/blob/52bd105d7175d1750e00344f41e12cc7cf76d188/src/OSVRTrackedHMD.cpp#L241
