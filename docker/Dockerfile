FROM walberla/buildenv-ubuntu-basic:16.04

MAINTAINER hradec <hradec@hradec.com>

ARG GCC_VERSION=4.9

RUN apt-get update 

RUN  apt-get install  -y \
    gcc-$GCC_VERSION \
    g++-$GCC_VERSION \
    nano \
    sudo \
 && apt-get clean \
 && rm -rf /var/lib/apt/lists/* 

RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-$GCC_VERSION 999 \
 && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-$GCC_VERSION 999 \
 && update-alternatives --install /usr/bin/cc  cc  /usr/bin/gcc-$GCC_VERSION 999 \
 && update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++-$GCC_VERSION 999

ENV CC="ccache gcc" CXX="ccache g++" 

ENV USER=testuser


RUN echo '#!/bin/bash' > /run.sh ; \
    echo 'runuser - $USER -c "cd /tmp/dev && mkdir build && cd build && cmake ../ && make"' >> /run.sh ; \
    chmod a+x /run.sh ; \
    cp /run.sh /start.sh

CMD ["/start.sh"]





