#!/bin/bash

DIR=$(dirname "$(readlink -f "$0")")

source "$DIR"/paths.sh

if [ ! -f "$VRPATHREG" ]; then
	echo "Please install SteamVR such that $VRPATHREG exists"
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
