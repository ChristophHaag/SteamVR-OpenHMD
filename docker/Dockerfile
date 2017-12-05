FROM walberla/buildenv-ubuntu-basic:16.04

MAINTAINER hradec <hradec@hradec.com>

ARG GCC_VERSION=4.9

RUN apt-get update

RUN  apt-get install  -y \
    gcc-$GCC_VERSION \
    g++-$GCC_VERSION \
    libhidapi-dev \
    libhidapi-libusb0 \
    nano \
    sudo

RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-$GCC_VERSION 999 \
 && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-$GCC_VERSION 999 \
 && update-alternatives --install /usr/bin/cc  cc  /usr/bin/gcc-$GCC_VERSION 999 \
 && update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++-$GCC_VERSION 999

ENV CC="ccache gcc" CXX="ccache g++ -std=c++11"

ENV USER=testuser

ADD start.sh /

CMD ["/start.sh"]
