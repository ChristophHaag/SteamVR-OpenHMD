#!/bin/bash

DIR=$(dirname "$(readlink -f "$0")")
VRPATHREG="$HOME"/.local/share/Steam/steamapps/common/SteamVR/bin/linux64/vrpathreg

CURRENTCONFIG="$HOME"/.local/share/Steam/config/steamvr.vrsettings
BACKUPDIR="$DIR"/steamvr-config-backup
BACKUPCONFIG="$BACKUPDIR"/steamvr.vrsettings
OHMDCONFIG="$DIR"/steamvr.vrsettings
OPENVR_API_PATH="$HOME"/.local/share/Steam/steamapps/common/SteamVR/bin/linux64/

if [ ! -f "$VRPATHREG" ]; then
	echo "Please install SteamVR so that $VRPATHREG exists"
	exit 1
fi

if [ -f "$BACKUPCONFIG" ]; then
	echo "Found backed up config in $BACKUPCONFIG"
	echo "Restoring SteamVR config $BACKUPCONFIG..."
	cp "$BACKUPCONFIG" "$CURRENTCONFIG"
	echo "Restored config!"
fi

echo "Unregistering driver..."
LD_LIBRARY_PATH="$OPENVR_API_PATH:$LD_LIBRARY_PATH" "$VRPATHREG" removedriver "$DIR"/build
echo "Unregistered driver!"
