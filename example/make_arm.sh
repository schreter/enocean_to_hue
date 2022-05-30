#!/bin/sh

# Destination machine where to install the executable
DEST=dm920
# Configuration to use
CONFIG=example/example.conf

# we expect the build directory to be at ../build.arm
ROOT=`dirname $0`/..
cd ${ROOT}/../build.arm || (echo "Could not find ARM build directory"; exit 1)

SOURCE=`grep enocean_to_hue_SOURCE_DIR CMakeCache.txt | cut -f2 -d=` || (echo "Cannot determine source directory"; exit 1)

make || (echo "Cannot build"; exit 1)
scp enocean_to_hue ${SOURCE}/example/example.conf ${DEST}:~ && ssh ${DEST} ./install_enocean.sh
