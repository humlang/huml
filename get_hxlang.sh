#!/usr/bin/env bash

# assume that you have cloned hx-lang and are in the respective directory

git submodule update --init --recursive

git submodule update --remote

cmake -B./lib/Catch2/build -H./lib/Catch2/ -DBUILD_TESTING=OFF -DCMAKE_INSTALL_PREFIX=./catch_install

cd lib/Catch2/build

make install

cd ../../..

cmake -B./lib/fmt/build -H./lib/fmt -DFMT_TEST=OFF

cd lib/fmt/build

make -j8

cd ../../..

cmake -B./build -H. -Dfmt_DIR=./lib/fmt/build -DCatch2_DIR=$(find catch_install/ -name "ParseAndAddCatchTests.cmake" | xargs dirname | head -n 1)

cd build

make -j8

cd ..

