sdbus-c++
=========

![ci](https://github.com/Kistler-Group/sdbus-cpp/workflows/CI/badge.svg)
![license](https://img.shields.io/github/license/Kistler-Group/sdbus-cpp)
![release](https://img.shields.io/github/v/release/Kistler-Group/sdbus-cpp)

sdbus-c++ is a high-level C++ D-Bus library for Linux designed to provide expressive, easy-to-use API in modern C++. It adds another layer of abstraction on top of sd-bus, a nice, fresh C D-Bus implementation by systemd.

sdbus-c++ has been written primarily as a replacement of dbus-c++, which currently suffers from a number of (unresolved) bugs, concurrency issues and inherent design complexities and limitations. sdbus-c++ has learned from dbus-c++ and has chosen a different path, a path of simple yet powerful design that is intuitive and friendly to the user and inherently free of those bugs.

Even though sdbus-c++ uses sd-bus library, it is not necessarily constrained to systemd and can perfectly be used in non-systemd environments as well.

Building and installing the library
-----------------------------------

The library is built using CMake:

```bash
$ mkdir build
$ cd build
$ cmake .. -DCMAKE_BUILD_TYPE=Release ${OTHER_CONFIG_FLAGS}
$ cmake --build .
$ sudo cmake --build . --target install
```

### CMake configuration flags for sdbus-c++

* `BUILD_CODE_GEN` [boolean]

  Option for building the stub code generator `sdbus-c++-xml2cpp` for generating the adaptor and proxy interfaces out of the D-Bus IDL XML description. Default value: `OFF`. Use `-DBUILD_CODE_GEN=ON` flag to turn on building the code gen.

* `BUILD_DOC` [boolean]

  Option for including sdbus-c++ documentation files and tutorials. Default value: `ON`. With this option turned on, you may also enable/disable the following option:

    * `BUILD_DOXYGEN_DOC` [boolean]

      Option for building Doxygen documentation of sdbus-c++ API. If enabled, the documentation must still be built explicitly through `cmake --build . --target doc`. Default value: `OFF`. Use `-DBUILD_DOXYGEN_DOC=OFF` to disable searching for Doxygen and building Doxygen documentation of sdbus-c++ API.

* `BUILD_TESTS` [boolean]

  Option for building sdbus-c++ unit and integration tests, invokable by `cmake --build . --target test` (Note: before invoking `cmake --build . --target test`, make sure you copy `tests/integrationtests/files/org.sdbuscpp.integrationtests.conf` file to `/etc/dbus-1/system.d` directory). That incorporates downloading and building static libraries of Google Test. Default value: `OFF`. Use `-DBUILD_TESTS=ON` to enable building the tests. With this option turned on, you may also enable/disable the following options:

    * `ENABLE_PERF_TESTS` [boolean]

      Option for building sdbus-c++ performance tests. Default value: `OFF`.

    * `ENABLE_STRESS_TESTS` [boolean]

      Option for building sdbus-c++ stress tests. Default value: `OFF`.

    * `TESTS_INSTALL_PATH` [string]

      Path where the test binaries shall get installed. Default value: `/opt/test/bin`.

* `BUILD_LIBSYSTEMD` [boolean]

  Option for building libsystemd dependency library automatically when sdbus-c++ is built, and making libsystemd an integral part of sdbus-c++ library. Default value: `OFF`. Might be very helpful in non-systemd environments where libsystemd shared library is unavailable (see [Solving libsystemd dependency](docs/using-sdbus-c++.md#solving-libsystemd-dependency) for more information). With this option turned on, you may also provide the following configuration flag:

    * `LIBSYSTEMD_VERSION` [string]

      Defines version of systemd to be downloaded, built and integrated into sdbus-c++. Default value: `242`.

    * `LIBSYSTEMD_EXTRA_CONFIG_OPTS` [string]

      Additional options to be passed as-is to the libsystemd build system (meson for systemd v242) in its configure step. Can be used for passing e.g. toolchain file path in case of cross builds. Default value: empty.

* `CMAKE_BUILD_TYPE` [string]

  This is a CMake-builtin option. Set to `Release` to build sdbus-c++ for production use. Set to `Debug` if you want to help further develop (and debug) the library :)

* `BUILD_SHARED_LIBS` [boolean]

  This is a global CMake flag, promoted in sdbus-c++ project to a CMake option. Use this to control whether sdbus-c++ is built as either a shared or static library. Default value: `ON`.

* `BUILD_EXAMPLES` [boolean]

  Build example programs which are located in the _example_ directory. Examples are not installed. Default value: `OFF`

Dependencies
------------

* `C++17` - the library uses C++17 features.
* `libsystemd` - systemd library containing sd-bus implementation. This library is part of systemd. Systemd at least v236 is needed. (In case you have a non-systemd environment, don't worry, see [Solving libsystemd dependency](docs/using-sdbus-c++.md#solving-libsystemd-dependency) for more information.)
* `googletest` - google unit testing framework, only necessary when building tests, will be downloaded and built automatically.
* `pkgconfig` - required for sdbus-c++ to be able to find some dependency packages.
* `expat` - necessary when building xml2cpp code generator (`BUILD_CODE_GEN` option is ON).

Licensing
---------

The library is distributed under LGPLv2.1 license, with a specific exception for macro/template/inline code in library header files.

References/documentation
------------------------

* [Using sdbus-c++](docs/using-sdbus-c++.md) - *the* main, comprehensive tutorial on sdbus-c++
* [Systemd and dbus configuration](docs/systemd-dbus-config.md)
* [D-Bus Specification](https://dbus.freedesktop.org/doc/dbus-specification.html)
* [sd-bus Overview](http://0pointer.net/blog/the-new-sd-bus-api-of-systemd.html)

Contributing
------------

Contributions that increase the library quality, functionality, or fix issues are very welcome. To introduce a change, please submit a pull request with a description.

Contact
-------

https://github.com/Kistler-Group/sdbus-cpp
