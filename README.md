# About

This is an attempt at implementing an ANSI-compliant Smalltalk system by repurposing the [pLisp](https://github.com/shikantaza/pLisp/)  intrastructure. The project is in its very early days, so there is not much to see yet, except for a REPL with integer operations.

The target platform is x86_64 Linux for the first version.

# Prerequisites

* Flex
* Bison
* autoconf
* [LLVM/Clang](https://github.com/llvm/llvm-project/releases/download/llvmorg-11.0.0/clang+llvm-11.0.0-x86_64-linux-gnu-ubuntu-20.04.tar.xz)
* gcc
* g++

# Build Instructions

Navigate to the directory where the project has been cloned and run these commands

1. sh ./autogen.sh
2. ./configure LLVMDIR=<clang 11.0 directory>
3. make
4. make install

This will create an executable called 'smalltalk.
