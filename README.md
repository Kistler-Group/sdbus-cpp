sdbus-c++
=========

sdbus-c++ is a C++ D-Bus library meant to provide high-level, expressive, easy-to-use interfaces in modern C++. It adds another layer of abstraction on top of sd-bus, a nice, fresh C D-Bus implementation by systemd.

sdbus-c++ has been written primarily as a replacement of dbus-c++, which currently suffers from a number of (unsolved) bugs, concurrency issues and inherent design limitations. sdbus-c++ has learned from dbus-c++ and tries to come up with a simple design that is flexible and friendly to the user and inherently free of those bugs.

Building and installing the library
-----------------------------------

The library is built using CMake:

```bash
$ mkdir build
$ cd build
$ cmake .. ${CONFIGURE_FLAGS_IF_NECESSARY}
$ make
$ sudo make install
```

### CMake configuration flags

* `BUILD_CODE_GEN` [boolean]

  Option for building the stub code generator `sdbus-c++-xml2cpp` for generating the adaptor and proxy interfaces out of the D-Bus IDL XML description. Default value: `OFF`. Use `-DBUILD_CODE_GEN=ON` flag to turn on building the code gen.

* `BUILD_DOC` [boolean]

  Option for including sdbus-c++ documentation files and tutorials. Default value: `ON`. With this option turned on, you may also enable/disable the following option:

    * `BUILD_DOXYGEN_DOC` [boolean]

      Option for building Doxygen documentation of sdbus-c++ API. If enabled, the documentation must still be built explicitly through `make doc`. Default value: `OFF`. Use `-DBUILD_DOXYGEN_DOC=OFF` to disable searching for Doxygen and building Doxygen documentation of sdbus-c++ API.

* `BUILD_TESTS` [boolean]

  Option for building sdbus-c++ unit and integration tests, invokable by `make test`. That incorporates downloading and building static libraries of Google Test. Default value: `OFF`. Use `-DBUILD_TESTS=ON` to enable building the tests. With this option turned on, you may also enable/disable the following options:

    * `BUILD_PERF_TESTS` [boolean]

      Option for building sdbus-c++ performance tests. Default value: `OFF`.

    * `BUILD_STRESS_TESTS` [boolean]

      Option for building sdbus-c++ stress tests. Default value: `OFF`.

    * `TESTS_INSTALL_PATH` [string]

      Path where the test binaries shall get installed. Default value: `/opt/test/bin`.

Dependencies
------------

* `C++17` - the library uses C++17 `std::uncaught_exceptions()` feature. When building sdbus-c++ manually, make sure you use a compiler that supports that feature (gcc >= 6, clang >= 3.7)
* `libsystemd` - systemd library containing sd-bus implementation. This library is part of systemd. Systemd at least v236 is needed. (Non-systemd environments are also supported, see the [tutorial](docs/using-sdbus-c++.md#solving-libsystemd-dependency) for more information.)
* `googletest` - google unit testing framework, only necessary when building tests, will be downloaded and built automatically.

Licensing
---------

The library is distributed under LGPLv2.1 license.

References/documentation
------------------------

* [D-Bus Specification](https://dbus.freedesktop.org/docs/dbus-specification.html)
* [sd-bus Overview](http://0pointer.net/blog/the-new-sd-bus-api-of-systemd.html)
* [Tutorial: Using sdbus-c++](docs/using-sdbus-c++.md)
* [Systemd and dbus configuration](docs/systemd-dbus-config.md)

Contributing
------------

Contributions that increase the library quality, functionality, or fix issues are very welcome. To introduce a change, please submit a pull request with a description.

Contact
-------

https://github.com/Kistler-Group/sdbus-cpp
