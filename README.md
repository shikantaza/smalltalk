# About

This is an attempt at implementing an ANSI-compliant Smalltalk system by repurposing the [pLisp](https://github.com/shikantaza/pLisp/)  infrastructure. The project is in its early days, so there is not much to see yet, except for a bare bones UI (Transcript, Workspace, and Debugger) with these features:

* Class creation, with integers, arrays, and user-created objects permitted as instance and shared variables
   
  `Smalltalk createClass: #MyClass`
  
  `Smalltalk createClass: #MyClass parentClass: ParentClass`
  
  `Smalltalk addClassMethod: #hello toClass: MyClass withBody: [ 42 ]`
  
  `Smalltalk createClass: #Fact`
  
  `Smalltalk addClassMethod: #fact: toClass: Fact withBody:
      [ :n | (n = 0) ifTrue: [ ^ 1]. ^ (n * (self fact: (n - 1))) ]`
  
* Global objects
 
  `Smalltalk createGlobal: #x valued: 42`

  `Smalltalk createGlobal: #x`

* Integer operations (+, -, *, /)
* `Transcript>>show:` and `Transcript>>cr` for printing things to the Transcript window
* Exception handling (`signal/return/resume/retry/pass/outer/resignalAs`)
* Debugger with step into, step over, and step out of functionality
* Basic block operations (`value, value:, on:do:, ensure:, ifCurtailed:`)
* Arrays (`new:, at:put:, at:, size, do:`)

(Note: use Control-Backspace to delete stuff from the REPL)

The target platform is x86_64 Linux for the first version.

# Prerequisites

* Flex
* Bison
* autoconf
* libtool
* libgc-dev
* libncurses-dev
* [LLVM/Clang](https://github.com/llvm/llvm-project/releases/download/llvmorg-11.0.0/clang+llvm-11.0.0-x86_64-linux-gnu-ubuntu-20.04.tar.xz)
* gcc
* g++
* libgtk-3-dev
* libgtksourceview-3.0-dev

# Build Instructions

Navigate to the directory where the project has been cloned and run these commands:

1. sh ./autogen.sh
2. ./configure LLVMDIR=<clang 11.0 directory>
3. make
4. make install

This will create an executable called 'smalltalk'.
