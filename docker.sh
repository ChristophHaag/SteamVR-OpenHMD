#!/bin/bash

CD=$(dirname $BASH_SOURCE)

cd $CD
CD=$(pwd)
cd $CD/docker

#docker-compose up --build
docker build ./ -t steamvr_openhmd/build

docker run -ti --rm \
    -e USER=$USER \
    -v $HOME:/home/$USER \
    -v $CD:/tmp/dev/ \
    -v /etc/passwd:/etc/passwd \
    --name build \
steamvr_openhmd/build:latest "$@"


~/.local/share/Steam/steamapps/common/SteamVR/bin/linux64/vrpathreg adddriver $CD/build/


export STEAM_RUNTIME_PREFER_HOST_LIBRARIES=0

~/.local/share/Steam/ubuntu12_32/steam-runtime/run.sh ~/.local/share/Steam/steamapps/common/SteamVR/bin/vrstartup.sh
