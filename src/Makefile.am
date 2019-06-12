AM_CFLAGS=-I$(top_srcdir)/inc

include_HEADERS = ../inc/edbase.h \
				  ../inc/edobject.h \
				  ../inc/list.h \
				  ../inc/edloop.h

lib_LTLIBRARIES = libedloop.la

libedloop_la_SOURCES = edobject.c \
					   edutil.c \
					   edloop.c \
					   edev_process.c \
					   edev_oneshot.c \
					   edev_timeout.c \
					   edev_ioevent.c

libedloop_la_CFLAGS  = $(AM_CFLAGS)
libedloop_la_LDFLAGS = -version-info @LIBEDLOOP_VERSION_INFO@
libedloop_la_LIBADD  =

if BUILD_EDEV_MQUEUE
include_HEADERS += ../inc/edev_mqueue.h
libedloop_la_SOURCES += edev_mqueue.c
endif

