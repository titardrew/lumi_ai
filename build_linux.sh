#!/bin/bash
# Recommended way to run:
#   chmod u+x ./build_linux.sh
#   ./build_linux.sh
set -e
set -x

build_folder="build_linux"
install_folder="Linux"

mkdir -p $build_folder

cmake -DCMAKE_BUILD_TYPE=Release -B $build_folder -S .
cmake --build $build_folder

rm -rf $install_folder
mkdir -p $install_folder
cp -r $build_folder/libOVYoloCpp.so $install_folder/

cd $install_folder
ldd libOVYoloCpp.so | awk 'NF == 4 { system("cp " $3 " .") }'
cd ..

cp /opt/intel/openvino_2022/runtime/lib/intel64/* $install_folder
