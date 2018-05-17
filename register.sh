#!/bin/bash

DIR=$(dirname "$(readlink -f "$0")")
VRPATHREG="$HOME"/.local/share/Steam/SteamApps/common/SteamVR/bin/linux64/vrpathreg

CURRENTCONFIG="$HOME"/.local/share/Steam/config/steamvr.vrsettings
BACKUPDIR="$DIR"/steamvr-config-backup
mkdir -p "$BACKUPDIR"
OHMDCONFIG="$DIR"/steamvr.vrsettings
OPENVR_API_PATH="$HOME"/.local/share/Steam/ubuntu12_32/

if [ ! -d "$DIR"/build ]; then
	echo "Please build in $DIR/build for this script to work"
	exit 1
fi

if [ ! -f "$VRPATHREG" ]; then
	echo "Please install SteamVR so that $VRPATHREG exists"
	exit 1
fi

if [ -f "$CURRENTCONFIG" ]; then
	echo "Found config in $CURRENTCONFIG"
	echo "Backing up current config to $BACKUPDIR..."
	mv "$CURRENTCONFIG" "$BACKUPDIR"
	echo "Backed up config!"
fi

echo "Installing SteamVR-OpenHMD config..."
cp "$OHMDCONFIG" "$CURRENTCONFIG"
echo "Installed SteamVR-OpenHMD config"

echo "Registering Driver..."
LD_LIBRARY_PATH="$OPENVR_API_PATH:$LD_LIBRARY_PATH" "$VRPATHREG" adddriver "$DIR"/build
