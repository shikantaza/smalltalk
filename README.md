# About

This is an attempt at implementing an ANSI-compliant Smalltalk system by repurposing the [pLisp](https://github.com/shikantaza/pLisp/)  infrastructure. The project is in its early days, so there is not much to see yet, except for a REPL with these features:

* Class creation, with integers and user-created objects permitted as instance and shared variables
  `Smalltalk createClass: #MyClass`
  `Smalltalk createClass: #MyClass parentClass: #ParentClass`
  `Smalltalk addClassMethod: #hello withBody: [ 42 ] toClass: #MyClass`
  `Smalltalk createClass: #Fact`
  `Smalltalk addClassMethod: #fact: withBody: [ :n | (n = 0) ifTrue: [ ^ 1]. ^ (n * (self fact: (n - 1))) ] toClass: #Fact`
* Global objects
  `Smalltalk createGlobal: #x valued: 42`
* Integer operations (+, -, *, /)
* `Transcript>>show:` and `Transcript>>cr` for printing strings to the REPL
* Exception handling (`signal/return/resume/pass/outer/resignalAs`)
* Basic block operations (`value, value:, on:do, ensure:, ifCurtailed:`)

(Note: use Control-Backspace to delete stuff from the REPL)

The target platform is x86_64 Linux for the first version.

# Prerequisites

* Flex
* Bison
* autoconf
* [LLVM/Clang](https://github.com/llvm/llvm-project/releases/download/llvmorg-11.0.0/clang+llvm-11.0.0-x86_64-linux-gnu-ubuntu-20.04.tar.xz)
* gcc
* g++

# Build Instructions

Navigate to the directory where the project has been cloned and run these commands:

1. sh ./autogen.sh
2. ./configure LLVMDIR=<clang 11.0 directory>
3. make
4. make install

This will create an executable called 'smalltalk'.
