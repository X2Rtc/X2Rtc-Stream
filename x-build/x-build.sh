#!/bin/sh

##https://blog.csdn.net/pompousx/article/details/107715000

DIR=$(pwd)
XSRT=
XSSL=

if [ "$2" = "aarch" ]; then
XSRT=--with-compiler-prefix=/home/xarm-toolchain/gcc-linaro-4.9-2016.02-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-
XSSL=linux-aarch64 no-asm no-async CROSS_COMPILE=/home/xarm-toolchain/gcc-linaro-4.9-2016.02-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-
fi

cd srt-1.5.2-rc.2
./configure --prefix=$DIR/build $XSRT
make && make install
cd -

cd openssl-3.0.7
./configure --prefix=$DIR/build $XSSL
make && make install
cd -

