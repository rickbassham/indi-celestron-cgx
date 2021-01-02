FROM rickbassham/raspbian-build:latest

RUN chroot /raspbian qemu-arm-static /bin/bash -c 'wget -O - https://www.astroberry.io/repo/key | apt-key add -'
RUN chroot /raspbian qemu-arm-static /bin/bash -c 'echo "deb https://www.astroberry.io/repo/ buster main" > /etc/apt/sources.list.d/astroberry.list'
RUN chroot /raspbian qemu-arm-static /bin/bash -c 'apt-get update && apt-get upgrade -y && rm -rf /var/lib/apt/lists/*'

RUN chroot /raspbian qemu-arm-static /bin/bash -c 'apt-get update && apt-get install -y libindi-dev libnova-dev libz-dev libgsl-dev && rm -rf /var/lib/apt/lists/*'

RUN mkdir -p /raspbian/src/indi-celestron-cgx
WORKDIR /raspbian/src/indi-celestron-cgx

COPY . /raspbian/src/indi-celestron-cgx

RUN mkdir -p /raspbian/build/deb-indi-celestron-cgx
WORKDIR /raspbian/build/deb-indi-celestron-cgx

RUN cp -r /raspbian/src/indi-celestron-cgx .
RUN cp -r /raspbian/src/indi-celestron-cgx/debian debian
