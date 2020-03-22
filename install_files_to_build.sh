#!/bin/sh

cp driver.vrdrivermanifest build/
cp -r resources build/

mkdir -p build/bin/{win64,win32,osx64,osx32,linux64,linux32}

cp build/driver_openhmd.so build/bin/linux64 2> /dev/null && \
mkdir -p build/bin/linux64/subprojects/openhmd && cp build/subprojects/openhmd/libopenhmd.so.0 build/bin/linux64/subprojects/openhmd && \
  echo "Installed linux plugin!" || echo "Did not install linux plugin!"

cp build/driver_openhmd.dll build/bin/win64 2> /dev/null && echo "Installed windows plugin!" || echo "Did not install windows plugin!"
