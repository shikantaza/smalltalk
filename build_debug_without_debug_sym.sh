#!/bin/sh
make distclean
./configure CFLAGS="-g" LLVMDIR=/data/clang+llvm-11.0.0-x86_64-linux-gnu-ubuntu-20.04/
make
