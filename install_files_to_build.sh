#!/bin/sh

cp driver.vrdrivermanifest build/
cp -r resources build/

mkdir -p build/bin/{win64,win32,osx64,osx32,linux64,linux32}

rm -f build/bin/linux64/driver_openhmd.so && cp build/driver_openhmd.so build/bin/linux64 2> /dev/null && \
  echo "Installed linux plugin!" || echo "Did not install linux plugin!"

rm -f build/bin/win64/driver_openhmnd.so && cp build/driver_openhmd.dll build/bin/win64 2> /dev/null && echo "Installed windows plugin!" || echo "Did not install windows plugin!"
