AC_PREREQ([2.59])
AC_INIT([edloop], [1.0.0], [robin.chen@wondalink.com])
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_SRCDIR([src/edloop.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIRS([m4])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES])

# Libtool shared library interface versions
LIBEDLOOP_VERSION_INFO="1:0:0"
AC_SUBST([LIBEDLOOP_VERSION_INFO])

# Checks for programs.
AC_PROG_CC
#AM_PROG_AR
AC_PROG_LIBTOOL
LT_INIT

# Build with -Wall.
CFLAGS="$CFLAGS -Wall"

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
AC_CHECK_FUNCS([epoll_create])
AC_CHECK_FUNCS([epoll_create1])

# Checks for features support.
AC_ARG_ENABLE([edev_mqueue],
  AS_HELP_STRING([--disable-edev-mqueue], [Disable the edev_mqueue component]))

# Conditionals for features support
AM_CONDITIONAL([BUILD_EDEV_MQUEUE], [test "x$enable_edev_mqueue" != "xno"])

# Defines for features support
AS_IF([test "x$enable_edev_mqueue" != "xno"], [AC_DEFINE([HAVE_EDEV_MQUEUE], [1], [Enable edev_mqueue])])

# Makefile
AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
