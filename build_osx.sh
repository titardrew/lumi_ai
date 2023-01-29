#!/bin/bash
# Recommended way to run:
#   chmod u+x ./build_osx.sh
#   ./build_osx.sh
set -e
set -x

mkdir -p build_osx
cd build_osx
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j4

#dylibbundler -of -cd  -b                                   \
#  -x ./OVYoloCpp.bundle/Contents/MacOS/OVYoloCpp           \
#  -p @loader_path/../libs/                                 \
#  -d ./OVYoloCpp.bundle/Contents/libs                      \
#  -s /opt/intel/openvino_2022/runtime/lib/intel64/Release/ \
#  -s /opt/intel/openvino_2022/runtime/3rdparty/tbb/lib

mkdir -p ./OVYoloCpp.bundle/Contents/libs/
install_name_tool -rpath /opt/intel/openvino_2022/runtime/lib/intel64/Release @loader_path/../libs ./OVYoloCpp.bundle/Contents/MacOS/OVYoloCpp

install_names ()
{
    extension=$1
    for file_path in `ls ./OVYoloCpp.bundle/Contents/libs/*.${extension}`
    do
        install_name_tool -add_rpath @loader_path/../libs/.     $file_path
        install_name_tool -add_rpath @loader_path/.             $file_path
    done
}

cp /opt/intel/openvino_2022/runtime/lib/intel64/Release/* ./OVYoloCpp.bundle/Contents/libs/
cp /opt/intel/openvino_2022/runtime/3rdparty/tbb/lib/* ./OVYoloCpp.bundle/Contents/libs/
install_names so
install_names dylib

cd ..
rm -rf macOS
mkdir -p macOS
cp -r build_osx/OVYoloCpp.bundle macOS/