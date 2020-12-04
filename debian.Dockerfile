FROM debian:buster

ARG DEBIAN_FRONTEND="noninteractive"
ENV TZ=America/New_York

RUN apt-get update && apt-get install -y \
    build-essential devscripts debhelper fakeroot cdbs software-properties-common cmake wget \
    && rm -rf /var/lib/apt/lists/*

RUN wget -O - https://www.astroberry.io/repo/key | apt-key add -

RUN echo 'deb https://www.astroberry.io/repo/ buster main' > /etc/apt/sources.list.d/astroberry.list

RUN apt-get update && apt-get install -y \
    libindi-dev libnova-dev libz-dev libgsl-dev \
    && rm -rf /var/lib/apt/lists/*

RUN mkdir -p /src/indi-celestron-cgx
WORKDIR /src/indi-celestron-cgx

COPY . /src/indi-celestron-cgx

RUN mkdir -p /build/deb-indi-celestron-cgx
WORKDIR /build/deb-indi-celestron-cgx

RUN cp -r /src/indi-celestron-cgx .
RUN cp -r /src/indi-celestron-cgx/debian debian
