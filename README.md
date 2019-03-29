sdbus-c++
=========

sdbus-c++ is a C++ API library for D-Bus IPC, based on sd-bus implementation.

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

  Option for building Doxygen documentation of sdbus-c++ API. If `BUILD_DOC` is enabled, the documentation must still be built explicitly through `make doc`. Default value: `ON`. Use `-DBUILD_DOC=OFF` to disable searching for Doxygen and building Doxygen documentation of sdbus-c++ API.

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

* `C++17` - the library uses C++17 `std::uncaught_exceptions()` feature. When building sdbus-c++ manually, make sure you use a compiler that supports that feature.
* `libsystemd` - systemd library containing sd-bus implementation. Systemd v236 at least is needed for sdbus-c++ to compile.
* `googletest` - google unit testing framework, only necessary when building tests, will be downloaded and built automatically

Licensing
---------

The library is distributed under LGPLv2.1 license.

References/documentation
------------------------

* [D-Bus Specification](https://dbus.freedesktop.org/doc/dbus-specification.html)
* [sd-bus Overview](http://0pointer.net/blog/the-new-sd-bus-api-of-systemd.html)
* [Tutorial: Using sdbus-c++](doc/using-sdbus-c++.md)
* [Systemd and dbus configuration](doc/systemd-dbus-config.md)

Contributing
------------

Contributions that increase the library quality, functionality, or fix issues are very welcome. To introduce a change, please submit a pull request with a description.

Contact
-------

https://github.com/Kistler-Group/sdbus-cpp
