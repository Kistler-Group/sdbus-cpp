EXTRA_DIST = autogen.sh
SUBDIRS = include src test 

DISTCHECK_CONFIGURE_FLAGS=

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = sdbus-c++.pc

CLEANFILES = *~ *.lo *.la

MOSTLYCLEANFILES = *.o

DISTCLEANFILES = \
	*libtool* \
	aclocal.m4 \
	compile config.* configure \
	depcomp install-sh \
	ltmain.sh \
	Makefile Makefile.in \
	missing \
	stamp-h1 \
	sdbus-c++.pc
