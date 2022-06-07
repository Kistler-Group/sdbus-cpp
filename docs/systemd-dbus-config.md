Systemd and D-Bus configuration
=======================

**Table of contents**

1. [Introduction](#introduction)
2. [Systemd configuration](#systemd-configuration)
3. [Dbus configuration](#dbus-configuration)

Introduction
------------

To run executable as a systemd service you may need some additional setup. For example, you may need explicitly allow the usage of your service. Following chapters contain template configurations.


Systemd configuration
---------------------------------------

Filename should use `.service` extension. It also must be placed in configuration directory (/etc/systemd/system in Ubuntu 18.04.1 LTS)

```
[Unit]
Description=nameOfService

[Service]
ExecStart=/path/to/executable

[Install]
WantedBy=multi-user.target
```

D-Bus configuration
------------------

Typical default D-Bus configuration does not allow to register services except explicitly allowed. To allow a service to register its D-Bus API, we must place an appropriate conf file in `/etc/dbus-1/system.d/` directory. The conf file name must be `<service-name>.conf`. I.e., full file path for Concatenator example from sdbus-c++ tutorial would be `/etc/dbus-1/system.d/org.sdbuscpp.concatenator.conf`. And here is template configuration to use its D-Bus interface under root:

```
<!DOCTYPE busconfig PUBLIC
 "-//freedesktop//DTD D-BUS Bus Configuration 1.0//EN"
 "http://www.freedesktop.org/standards/dbus/1.0/busconfig.dtd">
<busconfig>
  <policy user="root">
    <allow own="org.sdbuscpp.concatenator"/>
    <allow send_destination="org.sdbuscpp"/>
    <allow send_interface="org.sdbuscpp.concatenator"/>
  </policy>
</busconfig>
```

If you need access from other user then `root` should be substituted by desired username. Or you can simply use policy `<policy context="default">` like [conf file](/tests/integrationtests/files/org.sdbuscpp.integrationtests.conf) for sdbus-c++ integration tests is doing it. For more information refer to `man dbus-daemon`.
