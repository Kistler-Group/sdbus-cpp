# Defines how to build and install libsdbus-c++ tests

AUTOMAKE_OPTIONS = subdir-objects

# Target dirs for test binaries, scripts and files
testbindir = /opt/test/bin
dbusconfdir = $(sysconfdir)/dbus-1/system.d

# ENABLE_TESTS is defined by configure when user enables tests during configuration
if ENABLE_TESTS
testbin_PROGRAMS = libsdbus-c++_unittests libsdbus-c++_integrationtests
dbusconf_DATA = integrationtests/files/libsdbus-cpp-test.conf
endif

# Setting per-file flags
AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/src
AM_CXXFLAGS = @libsdbus_cpp_CFLAGS@ @SYSTEMD_CFLAGS@ -W -Wall -Werror -pedantic -pipe -std=c++17
AM_LDFLAGS = @libsdbus_cpp_LIBS@ @SYSTEMD_LIBS@

CLEANFILES = *~ *.lo *.la
MOSTLYCLEANFILES = *.o

TESTS =

# Configuration for libsdbus-c++_unittests
libsdbus_c___unittests_SOURCES =       \
unittests/libsdbus-c++_unittests.cpp   \
unittests/TypeTraits_test.cpp \
unittests/Types_test.cpp \
unittests/Message_test.cpp

libsdbus_c___unittests_LDFLAGS = -L$(top_builddir)/src -pthread

libsdbus_c___unittests_LDADD = \
	-lsdbus-c++ \
	@libsdbus_cpp_LIBS@  \
	@SYSTEMD_LIBS@ \
	-lgmock

TESTS += libsdbus-c++_unittests

# Configuration for libsdbus-c++_integrationtests
libsdbus_c___integrationtests_SOURCES =            \
integrationtests/libsdbus-c++_integrationtests.cpp \
integrationtests/Connection_test.cpp \
integrationtests/AdaptorAndProxy_test.cpp

libsdbus_c___integrationtests_LDFLAGS = -L$(top_builddir)/src -pthread

libsdbus_c___integrationtests_LDADD = \
	-lsdbus-c++ \
	@libsdbus_cpp_LIBS@  \
	@SYSTEMD_LIBS@ \
	-lgmock

TESTS += libsdbus-c++_integrationtests

check_PROGRAMS = libsdbus-c++_unittests libsdbus-c++_integrationtests

DISTCLEANFILES = Makefile Makefile.in

# Post-build action: executing tests from the IDE
if ENABLE_TESTS
all-local: libsdbus-c++_unittests libsdbus-c++_integrationtests
	if [ "${UNIT_TESTS_RUNNER}" ]; then "${UNIT_TESTS_RUNNER}" --deviceip="${TEST_DEVICE_IP}" --testbin=.libs/libsdbus-c++_unittests; fi; exit 0
	if [ "${UNIT_TESTS_RUNNER}" ]; then "${UNIT_TESTS_RUNNER}" --deviceip="${TEST_DEVICE_IP}" --testbin=.libs/libsdbus-c++_integrationtests; fi; exit 0
endif
