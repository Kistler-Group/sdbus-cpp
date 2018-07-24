AC_PREREQ(2.61)

# package version number (not shared library version)
# odd micro numbers indicate in-progress development
# even micro numbers indicate released versions
m4_define(sdbus_cpp_version_major, 0)
m4_define(sdbus_cpp_version_minor, 3)
m4_define(sdbus_cpp_version_micro, 1)

m4_define([sdbus_cpp_version],
          [sdbus_cpp_version_major.sdbus_cpp_version_minor.sdbus_cpp_version_micro])
m4_define([sdbus_cpp_api_version],
          [sdbus_cpp_version_major.sdbus_cpp_version_minor])

AC_INIT(libsdbus-c++, sdbus_cpp_version)
AM_INIT_AUTOMAKE
AC_CONFIG_HEADERS(config.h)

# Checks for programs.
AC_PROG_LIBTOOL
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL

# enable pkg-config
PKG_PROG_PKG_CONFIG

PKG_CHECK_MODULES(SYSTEMD, [libsystemd >= 236],,
    AC_MSG_ERROR([You need the libsystemd library (version 236 or newer)]
    [https://www.freedesktop.org/wiki/Software/systemd/])
)

# Checks for library functions.
#AC_CHECK_FUNCS([memset])

AC_SUBST(libsdbus_cpp_CFLAGS)
AC_SUBST(libsdbus_cpp_LIBS)

AC_ARG_ENABLE([tests],
  [AS_HELP_STRING([--enable-tests],
    [build and install tests @<:@default=yes@:>@])],
  [],
  [enable_tests=yes])
AM_CONDITIONAL([ENABLE_TESTS], [test x$enable_tests = xyes])

#icondir=${datadir}/icons/hicolor/32x32/apps
#AC_SUBST(icondir)

AC_OUTPUT([
Makefile
include/Makefile
src/Makefile
test/Makefile
sdbus-c++.pc
])

echo ""
echo "   sdbus-cpp $VERSION"
echo "   ====================="
echo ""
echo "   To build the project, run \"make\""
echo ""
