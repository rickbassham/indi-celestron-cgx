# Celestron CGX USB driver

This is an INDI driver for the Celestron CGX and CGX-L. It uses the USB port on the mount,
and does NOT require the hand controller. The time used for alignment purposes is system
time, so be sure whatever system you are running the driver on has a good time source.

This driver has only been tested in the northern hemisphere, and will likely not work
at all in the southern. Pull requests for southern hemisphere support are welcome.

## Supported Features

* Park/UnPark
* Mount Model Alignment
* GoTo
* Sync
* Tracking
* Guiding

## Unsupported (at this time)

* PEC

## Usage in KStars

After connecting to the mount, in the INDI Control Panel, click the `Align` button.
This will send the mount to the switch position and give us a roughly known position.
For best results, use the `Mount Model Tool` in the KStars Alignment module to generate
3 or more sync points and run the tool. The more sync points you add, the more accurate
the mount model will be, which will give more precise GOTOs. After the model is built,
you should have accurate GOTOs. NOTE: Any time you click the `Align` button, the driver
will clear the existing model, and you'll need to re-sync to build it up again.

## Installation

### All

```bash
wget -qO - https://rickbassham.github.io/indi-celestron-cgx/public.key | sudo apt-key add -
```

### Ubuntu Focal (amd64/arm64)

```bash
echo "deb https://rickbassham.github.io/indi-celestron-cgx/repos/apt/focal focal main" | \
    sudo tee /etc/apt/sources.list.d/indi-celestron-cgx.list
```

### Ubuntu Groovy (amd64/arm64)

```bash
echo "deb https://rickbassham.github.io/indi-celestron-cgx/repos/apt/groovy groovy main" | \
    sudo tee /etc/apt/sources.list.d/indi-celestron-cgx.list
```

### Raspbian Buster

```bash
echo "deb https://rickbassham.github.io/indi-celestron-cgx/repos/apt/raspbian buster main" | \
    sudo tee /etc/apt/sources.list.d/indi-celestron-cgx.list
```

### All

```bash
sudo apt update && sudo apt install indi-celestron-cgx
```

Compiling on any linux system
=============================

The compilation is simple. You will need indi libraries installed. The best way
is to install libindi-dev package from the PPA. You may also want to have
indi-gpsd and gpsd packages installed (not strictly required). If you cannot use
the PPA you need to install libindi-dev from your distribution or compile the
indi libraries yourself using instructions from the INDI website. I have not
tested the backward compatibility but the driver should compile and work at
least with the 1.8.4 version of the library. My recommendation: use PPA if you
can. To compile the driver you will need also: cmake, cdbs, libindi-dev,
libnova-dev, zlib1g-dev. Run following commands (you can select other install
prefix):

```sh
sudo apt install build-essential devscripts debhelper fakeroot cdbs software-properties-common cmake
sudo add-apt-repository ppa:mutlaqja/ppa
sudo apt install libindi-dev libnova-dev libz-dev libgsl-dev
```

```sh
mkdir -p ~/Projects
cd ~/Projects
git clone https://github.com/rickbassham/indi-celestron-cgx.git
mkdir -p ~/Projects/build/indi-celestron-cgx
cd ~/Projects/build/indi-celestron-cgx
cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Debug ~/Projects/indi-celestron-cgx
make
```

You can run `sudo make install` optionally at the end if you like to have the driver
properly installed.

Building debian/ubuntu packages
===============================

To build the debian package you will need the debian packaging tools:
`build-essential, devscripts, debhelper, fakeroot, cdbs`

Create `package` directory at the same level as indilib directory with the
cloned source. Then execute:

```sh
mkdir -p ~/Projects/build/deb-indi-celestron-cgx
cd ~/Projects/build/deb-indi-celestron-cgx
cp -r ~/indi-celestron-cgx .
cp -r ~/indi-celestron-cgx/debian debian
fakeroot debian/rules binary
fakeroot debian/rules clean
```
this should produce two packages in the main build directory (above `package`),
which you can install with `sudo dpkg -i indi-celestron-cgx_*.deb`.

