dnl This file uses AC_CONFIG_MACRO_DIR, so we need Autoconf 2.58 or better.
AC_PREREQ([2.58])

dnl Initialize autoconf.
AC_INIT([Coincer], [0.0.99], [xhire@coincer.org], [coincer], [https://www.coincer.org/])

dnl Safety check - list a source file that wouldn't be in other directories:
AC_CONFIG_SRCDIR([src/coincerd.c])

dnl Put autotools auxiliary files in subdirectories to reduce clutter:
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADER([src/autoconfig.h])

dnl Enable "automake" to simplify creating makefiles:
AM_INIT_AUTOMAKE([subdir-objects -Wall -Werror])
AC_CONFIG_FILES([Makefile])

dnl Compiler flags
AM_CFLAGS="-std=c89 -Wall -Wextra"

dnl Checks for programs.
dnl C compiler
AC_PROG_CC

dnl Checks for libraries.
PKG_PROG_PKG_CONFIG
PKG_CHECK_MODULES([JANSSON], [jansson])
PKG_CHECK_MODULES([LIBEVENT], [libevent])

dnl Checks for header files.

dnl Checks for typedefs, structures, and compiler characteristics.

dnl Checks for library functions.

dnl Do final output.
AC_SUBST([AM_CFLAGS])
AC_OUTPUT
