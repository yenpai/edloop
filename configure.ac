AC_PREREQ([2.59])
AC_INIT([edloop], [1.0.0], [robin.chen@wondalink.com])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/edcore/edloop.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

# AM_SILENT_RULES request by automake 1.11.  [no] defaults V=1
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# Libtool shared library versions
AC_SUBST([LIBEDLOOP_VERSION_INFO], [1:0:0])

# Silence warning: ar: 'u' modifier ignored since 'D' is the default
AC_SUBST(AR_FLAGS, [cr])

# Use GNU extensions
AC_GNU_SOURCE

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_PROG_GCC_TRADITIONAL
AS_IF([test "$GCC" = "yes"], [CFLAGS="$CFLAGS -Wall"])
LT_INIT

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
AC_CHECK_FUNCS([eventfd])

# Checks for pthread support.
LIBS="${LIBS} -pthread"

# Checks for edev_mqueue support.
AC_ARG_ENABLE([edev_mqueue], AS_HELP_STRING([--enable-edev-mqueue], [enable edev_mqueue support [default=no]]))
AM_CONDITIONAL([BUILD_EDEV_MQUEUE], [test "$enable_edev_mqueue" = "yes"])
AS_IF([test "$enable_edev_mqueue" = "yes"], [AC_DEFINE([HAVE_EDEV_MQUEUE], [1], [Define to 1 if edev_mqueue support])])

# Checks for tests build
AC_ARG_ENABLE([tests_build], AS_HELP_STRING([--enable-tests-build], [build tests applications [default=no]]))
AM_CONDITIONAL([BUILD_TESTS], [test "$enable_tests_build" = "yes"])

# Makefile
AC_CONFIG_FILES([libedloop.pc])
AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([src/Makefile])
AC_CONFIG_FILES([src/edcore/Makefile])
AC_CONFIG_FILES([src/edev_process/Makefile])
AC_CONFIG_FILES([src/edev_oneshot/Makefile])
AC_CONFIG_FILES([src/edev_timeout/Makefile])
AC_CONFIG_FILES([src/edev_ioevent/Makefile])
AC_CONFIG_FILES([src/edev_mqueue/Makefile])
AC_CONFIG_FILES([tests/Makefile])

AC_OUTPUT
