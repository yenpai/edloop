AM_CFLAGS   = -Wall
AM_CPPFLAGS = -I$(top_srcdir)/src/edcore
LDADD       = ../src/libedloop.la

noinst_PROGRAMS  = test1
test1_SOURCES    = test1.c

if BUILD_EDEV_MQUEUE
noinst_PROGRAMS         += test_edev_mqueue
test_edev_mqueue_SOURCES = test_edev_mqueue.c
test_edev_mqueue_CFLAGS  = -I$(top_srcdir)/src/edev_mqueue
endif

if BUILD_EDEV_USBDEV
noinst_PROGRAMS         += test_edev_usbdev
test_edev_usbdev_SOURCES = test_edev_usbdev.c
test_edev_usbdev_CFLAGS  = -I$(top_srcdir)/src/edev_usbdev
endif
