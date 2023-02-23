#!/bin/sh

set -x

SOURCE_DIR=`pwd`
BUILD_DIR=${BUILD_DIR:-../build}
BUILD_TYPE=${BUILD_TYPE:-Debug}
INSTALL_DIR=${INSTALL_DIR:-$SOURCE_DIR}
CXX=${CXX:-g++}

mkdir -p $BUILD_DIR/$BUILD_TYPE \
      && cd $BUILD_DIR/$BUILD_TYPE \
      && cmake \
               -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
               -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR \
               $SOURCE_DIR \
      && make $*