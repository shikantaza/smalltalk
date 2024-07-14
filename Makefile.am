AM_YFLAGS = -d

all:	${bin_PROGRAMS} ${lib_LTLIBRARIES}

bin_PROGRAMS = smalltalk

BUILT_SOURCES = libsmalltalkjitllvm.la

smalltalk_SOURCES = src/parser.y src/parser_utils.c src/lex.l src/compiler.c src/object_utils.c src/lisp_compiler.c src/metacont.c src/c_compiler.c src/util.c src/message.c src/primitives.c src/smalltalk.c src/class_library/integer.c src/class_library/transcript.c


AM_CFLAGS = -DSMALLTALKDATADIR=\"$(pkgdatadir)\" -Wall

#set the --export-dynamic flag only for Linux (needed for LLVM)
if LINUX
AM_CFLAGS += -Wl,--export-dynamic
endif

if WINDOWS
AM_CFLAGS += -Wl,--export-all-symbols
endif

AM_LIBS = ${LEXLIB}

ACLOCAL_AMFLAGS = -I m4 --install


if WINDOWS
smalltalk_LDADD = ${AM_LIBS} -lgc $(LIBS) libsmalltalkjitllvm.la -lversion -lstdc++ -ldl
else
smalltalk_LDADD = ${AM_LIBS} -lgc $(LIBS) libsmalltalkjitllvm.la -ldl
endif

lib_LTLIBRARIES = libsmalltalkjitllvm.la

clang_LIBS = -lclangCodeGen  -lclangFrontend -lclangSerialization -lclangDriver -lclangParse -lclangSema -lclangAnalysis -lclangEdit -lclangAST -lclangLex -lclangBasic
libsmalltalkjitllvm_la_SOURCES = src/jit_llvm.cpp
libsmalltalkjitllvm_la_CXXFLAGS = -std=c++14 -fPIC -fno-rtti `$(LLVMDIR)/bin/llvm-config --cflags` 
libsmalltalkjitllvm_la_LDFLAGS = `$(LLVMDIR)/bin/llvm-config --ldflags` -no-undefined -avoid-version $(clang_LIBS) `$(LLVMDIR)/bin/llvm-config --libs` -lz -lncurses -pthread -ldl

