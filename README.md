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

By default, the library builds its unit and integration tests. That incorporates downloading and building static libraries of Google Test. Use `-DENABLE_TESTS=OFF` configure flag if you want to disable building the tests.

By default, the library doesn't build the code generator for adaptor and proxy interfaces. Use `-DBUILD_CODE_GEN=ON` flag to also build the code generator.

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
