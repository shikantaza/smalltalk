# Copyright 2026 Rajesh Jayaprakash <rajesh.jayaprakash@protonmail.com>

# This is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# It is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this file.  If not, see <http://www.gnu.org/licenses/>.

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
PKG_CHECK_MODULES([libffi], [libffi >= 3.0.11])

LT_INIT

AM_CONDITIONAL(LINUX, test `uname -s` = "Linux")
AM_CONDITIONAL(WINDOWS, test `uname -o` = "Msys")

AC_OUTPUT

AM_PROG_AR
