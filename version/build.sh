#!/usr/bin/env bash
rm -rf build/ httpServer
echo "创建编译目录：build"
mkdir build
echo "进入编译目录：build"
cd build/
echo "do:cmake ../src"
cmake ../src
echo "do:make"
make
echo "移动可执行文件httpServer到当前目录"
cp app/http/httpServer ../
echo "done..."