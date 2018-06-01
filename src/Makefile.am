
lib_LTLIBRARIES = libsdbus-c++.la

libsdbus_c___la_SOURCES = \
	Connection.cpp \
	ConvenienceClasses.cpp \
	Message.cpp \
	AsyncResult.cpp \
	Object.cpp \
	ObjectProxy.cpp \
	Types.cpp \
	Error.cpp \
	VTableUtils.c

libsdbus_c___la_LIBADD = @SYSTEMD_LIBS@

# Setting per-file flags
AM_CPPFLAGS = -I$(top_srcdir)/include
AM_CXXFLAGS = @libsdbus_cpp_CFLAGS@ @SYSTEMD_CFLAGS@ -std=c++17 -pipe -pedantic -W -Wall
AM_LDFLAGS = @libsdbus_cpp_LIBS@ @SYSTEMD_LIBS@ 

libsdbus_c___la_LDFLAGS = -version-info 0:0:0

# Cleaning
CLEANFILES = *~ *.lo *.la
MOSTLYCLEANFILES = *.o
DISTCLEANFILES = Makefile Makefile.in
