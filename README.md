sdbus-c++
=========

sdbus-c++ is a C++ API library for D-Bus IPC, based on sd-bus implementation.

Building and installing the library
-----------------------------------

```bash
$ ./autogen.sh ${CONFIGURE_FLAGS}
$ make
$ sudo make install
```

Use `--disable-tests` flag when configuring to disable building unit and integration tests for the library.

Dependencies
------------

* `C++17` - the library uses C++17 `std::uncaught_exceptions()` feature. When building sdbus-c++ manually, make sure you use a compiler that supports that feature.
* `libsystemd` - systemd library containing sd-bus implementation. Systemd v236 at least is needed for sdbus-c++ to compile.
* `googletest` - google unit testing framework, only necessary when building tests

Licensing
---------

The library is distributed under LGPLv2.1+ license.

References/documentation
------------------------

* [D-Bus Specification](https://dbus.freedesktop.org/doc/dbus-specification.html)
* [sd-bus Overview](http://0pointer.net/blog/the-new-sd-bus-api-of-systemd.html)
* [Tutorial: Using sdbus-c++](doc/using-sdbus-c++.md)

Contributing
------------

Contributions that increase the library quality, functionality, or fix issues are very welcome. To introduce a change, please submit a merge request with a description.

Contact
-------

stanislav.angelovic[at]kistler.com
