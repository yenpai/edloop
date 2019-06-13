lib_LTLIBRARIES = libedloop.la

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
libedloop_la_SOURCES += edev_mqueue.c
endif
