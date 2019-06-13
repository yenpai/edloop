AUTOMAKE_OPTIONS = foreign 1.9
ACLOCAL_AMFLAGS = -I m4
SUBDIRS = src

if BUILD_TESTS
SUBDIRS += tests
endif

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libedloop.pc
