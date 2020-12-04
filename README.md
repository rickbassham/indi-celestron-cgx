Celestron CGX USB driver
-------------------------------------

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
mkdir -p ~/Projects/build/indi-celestron-cgx
cd ~/Projects/build/indi-celestron-cgx
cmake -DCMAKE_INSTALL_PREFIX=/usr/local -DCMAKE_BUILD_TYPE=Debug ~/Projects/indi-celestron-cgx
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

