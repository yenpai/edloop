SUBDIRS = edcore edev_process edev_oneshot edev_timeout edev_ioevent

lib_LTLIBRARIES      = libedloop.la
libedloop_la_SOURCES = 
libedloop_la_CFLAGS  =
libedloop_la_LDFLAGS = -version-info $(LIBEDLOOP_VERSION_INFO)
libedloop_la_LIBADD  = edcore/libedcore.la \
					   edev_process/libedev_process.la \
					   edev_oneshot/libedev_oneshot.la \
					   edev_timeout/libedev_timeout.la \
					   edev_ioevent/libedev_ioevent.la

if BUILD_EDEV_MQUEUE
SUBDIRS += edev_mqueue
libedloop_la_LIBADD  += edev_mqueue/libedev_mqueue.la
endif

if BUILD_EDEV_HOTPLUG
SUBDIRS += edev_hotplug
libedloop_la_LIBADD  += edev_hotplug/libedev_hotplug.la
endif
