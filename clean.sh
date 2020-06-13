#!/usr/bin/bash

make clean
rm -rf CMakeFiles
rm CMakeCache.txt \
    cmake_install.cmake \
    CPackConfig.cmake \
    CPackSourceConfig.cmake \
    Makefile \
    *.cbp

rm -rf output/*

