AM_CFLAGS   = -Wall
AM_CPPFLAGS = -I$(top_srcdir)/src/edcore -I$(top_srcdir)/src/edev_mqueue
LDADD       = ../src/libedloop.la

#bin_PROGRAMS     = test1 
noinst_PROGRAMS  = test1
test1_SOURCES    = test1.c

if BUILD_EDEV_MQUEUE
#bin_PROGRAMS    += test2
noinst_PROGRAMS += test2
test2_SOURCES    = test2.c
endif
