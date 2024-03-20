#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([tc-mode-manager], [1.0.0], [limdh3@telechips.com])
AM_INIT_AUTOMAKE
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/main.c])

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_INSTALL
AC_PROG_CPP
AC_PROG_MKDIR_P

# Checks for pkgconfigs.
PKG_CHECK_EXISTS(libxml-2.0)
PKG_CHECK_EXISTS(glib-2.0)
PKG_CHECK_EXISTS(dbus-1)
PKG_CHECK_EXISTS(TcUtils)
PKG_CHECK_EXISTS(libsystemd)

PKG_CHECK_MODULES([MODEPKG], [glib-2.0 libxml-2.0 dbus-1 TcUtils libsystemd])

# Checks for libraries.
AC_CHECK_LIB([pthread], [pthread_join], , )
AC_CHECK_LIB([xml2], [main])
AC_CHECK_LIB([tcutils], [main])


# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_PID_T

# Checks for library functions.
AC_FUNC_FORK

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
