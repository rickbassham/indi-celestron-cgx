ARG UBUNTU_VERSION="focal"
FROM rickbassham/ubuntu-raspberry-build:${UBUNTU_VERSION}

RUN chroot /raspberry qemu-aarch64-static /bin/bash -c 'DEBIAN_FRONTEND="noninteractive" add-apt-repository ppa:mutlaqja/ppa && rm -rf /var/lib/apt/lists/*'
RUN chroot /raspberry qemu-aarch64-static /bin/bash -c 'apt-get update && DEBIAN_FRONTEND="noninteractive" apt-get upgrade -y && rm -rf /var/lib/apt/lists/*'

RUN chroot /raspberry qemu-aarch64-static /bin/bash -c 'apt-get update && DEBIAN_FRONTEND="noninteractive" apt-get install -y libindi-dev libnova-dev libz-dev libgsl-dev && rm -rf /var/lib/apt/lists/*'

RUN mkdir -p /raspberry/src/indi-celestron-cgx
WORKDIR /raspberry/src/indi-celestron-cgx

COPY . /raspberry/src/indi-celestron-cgx

RUN mkdir -p /raspberry/build/deb-indi-celestron-cgx
WORKDIR /raspberry/build/deb-indi-celestron-cgx

RUN cp -r /raspberry/src/indi-celestron-cgx .
RUN cp -r /raspberry/src/indi-celestron-cgx/debian debian
