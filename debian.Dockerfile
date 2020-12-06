FROM balenalib/raspberry-pi-debian:latest

RUN [ "cross-build-start" ]

ARG DEBIAN_FRONTEND="noninteractive"
ENV TZ=America/New_York

RUN install_packages \
    build-essential devscripts debhelper fakeroot cdbs software-properties-common cmake wget

#RUN wget -O - https://ppa.stellarmate.com/repos/apt/conf/sm.gpg.key | apt-key add -
#RUN echo 'deb https://ppa.stellarmate.com/repos/apt/stable/ buster main' > /etc/apt/sources.list.d/stellarmate.list

RUN wget -O - https://www.astroberry.io/repo/key | apt-key add -
RUN echo 'deb https://www.astroberry.io/repo/ buster main' > /etc/apt/sources.list.d/astroberry.list

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
