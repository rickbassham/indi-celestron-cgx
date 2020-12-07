ARG UBUNTU_VERSION=focal
FROM balenalib/aarch64-ubuntu:${UBUNTU_VERSION}-run

RUN [ "cross-build-start" ]

ARG DEBIAN_FRONTEND="noninteractive"
ENV TZ=America/New_York

RUN install_packages \
    build-essential devscripts debhelper fakeroot cdbs software-properties-common cmake wget

RUN add-apt-repository ppa:mutlaqja/ppa

RUN install_packages \
    libindi-dev libnova-dev libz-dev libgsl-dev

RUN mkdir -p /src/indi-celestron-cgx
WORKDIR /src/indi-celestron-cgx

COPY . /src/indi-celestron-cgx

RUN mkdir -p /build/deb-indi-celestron-cgx
WORKDIR /build/deb-indi-celestron-cgx

RUN cp -r /src/indi-celestron-cgx .
RUN cp -r /src/indi-celestron-cgx/debian debian

RUN [ "cross-build-end" ]
