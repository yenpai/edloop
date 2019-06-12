#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([edloop], [1.0.0])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])
AC_CONFIG_SRCDIR([src/edloop.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIRS([m4])

# Libtool shared library interface versions
LIBEDLOOP_VERSION_INFO="1:0:0"
AC_SUBST([LIBEDLOOP_VERSION_INFO])

# Checks for features.
AC_ARG_ENABLE([edev_mqueue],
  AS_HELP_STRING([--disable-edev-mqueue], [disable the edev_mqueue component]),
  [build_edev_mqueue="$enableval"], [build_edev_mqueue="yes"])
AM_CONDITIONAL([BUILD_EDEV_MQUEUE], [test x$build_edev_mqueue = xyes])

AS_IF([test "x$enable_edev_mqueue" != "xno"], [AC_DEFINE([HAVE_EDEV_MQUEUE], [1], [Enable edev_mqueue])])

# Checks for programs.
AC_PROG_CC
AM_PROG_AR

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h limits.h stddef.h stdint.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT8_T
AC_TYPE_PID_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([clock_gettime memset])

# Use libtool
LT_INIT

# Makefile
AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
