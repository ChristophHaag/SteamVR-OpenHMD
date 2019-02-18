#!/usr/bin/bash

DIR=$(dirname "$(readlink -f "$0")")

if [ -d "$HOME"/.local/share/Steam ]
then
	STEAMDIR="$HOME"/.local/share/Steam/
elif [ -d "$HOME"/.steam/steam ]
then
	STEAMDIR="$HOME"/.steam/steam
fi

if [ -d "$STEAMDIR"/steamapps/common/SteamVR ]
then
	STEAMVRDIR="$STEAMDIR"/steamapps/common/SteamVR
elif [ -d "$STEAMDIR"/SteamApps/common/SteamVR ]
then
	STEAMVRDIR="$STEAMDIR"/SteamApps/common/SteamVR
fi

VRPATHREG="$STEAMVRDIR"/bin/linux64/vrpathreg
CURRENTCONFIG="$STEAMDIR"/config/steamvr.vrsettings
BACKUPDIR="$DIR"/steamvr-config-backup
BACKUPCONFIG="$BACKUPDIR"/steamvr.vrsettings
OHMDCONFIG="$DIR"/steamvr.vrsettings
OPENVR_API_PATH="$STEAMVRDIR"/bin/linux64/
