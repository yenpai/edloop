lib_LTLIBRARIES = libedloop.la

include_HEADERS = $(top_srcdir)/inc/edbase.h   \
				  $(top_srcdir)/inc/edobject.h \
				  $(top_srcdir)/inc/list.h     \
				  $(top_srcdir)/inc/edloop.h

libedloop_la_SOURCES = edobject.c     \
					   edutil.c       \
					   edloop.c       \
					   edev_process.c \
					   edev_oneshot.c \
					   edev_timeout.c \
					   edev_ioevent.c

libedloop_la_CFLAGS  = -include $(top_builddir)/config.h -I$(top_srcdir)/inc
libedloop_la_LDFLAGS = -version-info $(LIBEDLOOP_VERSION_INFO)
libedloop_la_LIBADD  =

if BUILD_EDEV_MQUEUE
include_HEADERS += $(top_srcdir)/inc/edev_mqueue.h
libedloop_la_SOURCES += edev_mqueue.c
endif
