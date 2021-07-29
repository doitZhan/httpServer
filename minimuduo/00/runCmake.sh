#!/usr/bin/env bash
rm -rf build/ version*
echo "创建编译目录：build"
mkdir build
echo "进入编译目录：build"
cd build/
echo "do:cmake .."
cmake ..
echo "do:make"
make
echo "移动可执行文件到当前目录"
mv version* ../
echo "done..."