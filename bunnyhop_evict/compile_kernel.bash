#!/bin/bash

rm -f linux-*.deb
rm -f linux-5.12.4_1.colorKernel*
rm -rf kernel/
cp -r linux/ kernel/
#cp .config kernel/
cp /usr/src/linux-headers-$(uname -r)/.config kernel/ ### Or you could generate it with make menuconfig


cd kernel/
#scripts/config --set-str SYSTEM_TRUSTED_KEYS ""
#make menuconfig
scripts/config --set-str SYSTEM_TRUSTED_KEYS ""
make -j 12 KDEB_PKGVERSION=1.colorKernel deb-pkg
cd ../
