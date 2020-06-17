#!/usr/bin/bash

DIR=$(dirname "$(readlink -f "$0")")
STEAMDIR="C:/Program Files (x86)/Steam"
STEAMVRDIR="C:/Program Files (x86)/Steam/steamapps/common/SteamVR"
VRPATHREG="$STEAMVRDIR"/bin/win64/vrpathreg.exe
CURRENTCONFIG="$STEAMDIR"/config/steamvr.vrsettings
BACKUPDIR="$DIR"/steamvr-config-backup
BACKUPCONFIG="$BACKUPDIR"/steamvr.vrsettings
OHMDCONFIG="$DIR"/steamvr.vrsettings
OPENVR_API_PATH="$STEAMVRDIR"/bin/win64/
