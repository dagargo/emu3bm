#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.71])
AC_INIT([emu3bm],[1.0],[dagargo@gmail.com])
AC_CONFIG_SRCDIR([src])
AC_CONFIG_HEADERS([config.h])
LT_INIT
AC_CONFIG_MACRO_DIRS([m4])
AM_INIT_AUTOMAKE([subdir-objects])

# Checks for programs.
AC_PROG_CC
AC_PROG_YACC
AC_PROG_LEX([noyywrap])

PKG_CHECK_MODULES([GLIB], [glib-2.0])

# Checks for libraries.
PKG_CHECK_MODULES(SNDFILE, sndfile >= 1.0.2)
AC_SUBST(SNDFILE_CFLAGS)
AC_SUBST(SNDFILE_LIBS)

PKG_CHECK_MODULES(SAMPLERATE, samplerate >= 0.1.8)
AC_SUBST(SAMPLERATE_CFLAGS)
AC_SUBST(SAMPLERATE_LIBS)

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h unistd.h inttypes.h stdint.h math.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_FUNC_MALLOC

AC_CONFIG_FILES([Makefile src/Makefile res/Makefile man/Makefile test/Makefile])
AC_OUTPUT
