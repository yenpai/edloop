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

if BUILD_EDEV_HOTPLUG
noinst_PROGRAMS          += test_edev_hotplug
test_edev_hotplug_SOURCES = test_edev_hotplug.c
test_edev_hotplug_CFLAGS  = -I$(top_srcdir)/src/edev_hotplug
endif
