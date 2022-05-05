![Main Page](https://i.imgur.com/QOi0Yvv.jpg)

## Features
- Installs NSP/NSZ/XCI/XCZ files and split NSP/XCI files from your SD card.
- Installs NSP/NSZ/XCI/XCZ files over LAN or USB from tools such as [NS-USBloader](https://github.com/developersu/ns-usbloader).
- Installs Split NSP/XCI/NSZ/XCZ over Lan or USB using [NS-USBloader(Mod)](https://mega.nz/file/I4p2gCCK#32GwAGtIcL3FVH-V-8Goae_hpnK8FQ0eS2PwLDOW6X4).
- Installs NSP/NSZ/XCI/XCZ files over the internet by URL or Google Drive.
- Installs NSP/NSZ/XCI/XCZ files from a Hard Drive (NTFS/Fat32/ExFat/EXT3/EXT4).
- Verifies NCAs by header signature before they're installed.
- Installs and manages the latest signature patches quickly and easily.
- Works on SX OS and Atmosphere.
- Able to theme, change install sounds.

## Thanks to
- Blawar, Hunterweb, DarkMatterCore, XorTroll

## Modified Code
This code was prominently modified by MrDude on 25/04/2022 to be able to build with new plutonium and up to date libnx.

## Building All componenets of TinWoo at once
cd into the tinwoo folder then "make".

## Build TinWoo components individually
First, build and install usb libs - "make libusb".\
Second, built Plutonium - "make plutonium".\
Third, Make Tinwoo - "make tinwoo".

## Cleanup TinWoo once built
First, "make libusbclean".\
Second, "make cleanplutonium".\
Third, "make clean".

## Note
This is a work in progress and lets you build with new libnx, plutonium packages. Some stuff still needs fixed to work with the new plutonium and libnx changes.

## Stuff still to fix
~~All known bugs fixed~~ \

## Build Issues
Make sure you are using Libnx build at least 9865dbf9 version.

git clone --recursive https://github.com/switchbrew/libnx.git \
cd libnx \
git checkout 9865dbf9 \
make install

## Check libnx version
pacman -Q --info libnx
