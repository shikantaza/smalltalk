AM_YFLAGS = -d

all:	${bin_PROGRAMS} ${lib_LTLIBRARIES}

bin_PROGRAMS = smalltalk

BUILT_SOURCES = libsmalltalkjitllvm.la

smalltalk_SOURCES = src/parser.y src/parser_utils.c src/lex.l src/compiler.c src/object_utils.c src/lisp_compiler.c src/metacont.c src/c_compiler.c src/util.c src/message.c src/primitives.c src/smalltalk.c src/class_library/integer.c src/class_library/transcript.c src/class_library/niladic_block.c src/exceptions.c src/class_library/boolean.c src/stack.c src/class_library/monadic_block.c src/class_library/collections/array.c src/class_library/collections/ordered_collection.c src/gui/ui.c src/gui/event_handlers.c src/gui/code_renderer.c src/block_conversion.c src/class_library/readable_string.c src/class_library/character.c src/class_library/dyadic_valuable.c src/gui/class_browser.c

AM_CFLAGS = -DSMALLTALKDATADIR=\"$(pkgdatadir)\" ${gtk_CFLAGS} ${gtksourceview_CFLAGS} -Wall

#set the --export-dynamic flag only for Linux (needed for LLVM)
if LINUX
AM_CFLAGS += -Wl,--export-dynamic
endif

if WINDOWS
AM_CFLAGS += -Wl,--export-all-symbols
endif

AM_LIBS = ${gtk_LIBS} ${gtksourceview_LIBS} ${LEXLIB}

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

pkgdata_DATA = lib/smalltalk.st lib/smalltalk2.st lib/package.st data/smalltalk.lang

dist_pkgdata_DATA = ${pkgdata_DATA}

iconsdir = $(pkgdatadir)/icons
icons_DATA  = icons/load_image.png icons/save_image.png icons/workspace.png icons/browser.png icons/clear.png  \
              icons/exit.png icons/evaluate.png icons/load_file.png icons/file_browser.png icons/clear32x32.png \
              icons/exit32x32.png icons/resume_with_val32x32.png icons/resume32x32.png icons/retry32x32.png \
              icons/continue.png icons/abort.png icons/step_into.png icons/step_over.png icons/step_out.png \
              icons/accept.png icons/refresh.png icons/remove_breakpoint.png icons/remove_breakpoints.png

