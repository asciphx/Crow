#!/bin/bash
cd build && cmake .. -DCMAKE_TOOLCHAIN_FILE=/home/asciphx/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_INSTALL_PREFIX=$HOME/local && make -j4 install;
