AC_ARG_VAR(LLVMDIR, "Directory in which LLVM and Clang are installed")

AC_INIT([smalltalk], [0.1.0], [rajesh.jayaprakash@protonmail.com], [smalltalk], [https://github.com/shikantaza/smalltalk])
AC_PREREQ([2.68])
AC_CONFIG_SRCDIR([src/compiler.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([build-aux])

AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.11 -Wall subdir-objects])
AC_CONFIG_FILES([Makefile])
AC_PROG_CC
AC_PROG_LEX
AC_PROG_YACC
#AC_PROG_RANLIB
AM_PROG_CC_C_O
# Put various checks and such here.

AC_PROG_CXX

#AC_SEARCH_LIBS([dlopen], [dl], [], [])

PKG_CHECK_MODULES([gtk], [gtk+-3.0 >= 3.4.2])
PKG_CHECK_MODULES([gtksourceview], [gtksourceview-3.0 >= 3.4.2])
#PKG_CHECK_MODULES([libffi], [libffi >= 3.0.11])

LT_INIT

AM_CONDITIONAL(LINUX, test `uname -s` = "Linux")
AM_CONDITIONAL(WINDOWS, test `uname -o` = "Msys")

AC_OUTPUT

AM_PROG_AR
