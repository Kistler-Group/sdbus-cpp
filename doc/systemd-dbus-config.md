Systemd and dbus configuration
=======================

**Table of contents**

1. [Introduction](#introduction)
2. [Systemd configuration](#systemd-configuration)
3. [Dbus configuration](#dbus-configuration)

Introduction
------------

To run executable as a systemd service you may need some additional setup. For example, you may need explicitly allow 
the usage of your service. Following chapters contain template configurations.


Systemd configuration
---------------------------------------

Filename should use `.service` extension. It also must be placed in configuration directory (/etc/systemd/system in
Ubuntu 18.04.1 LTS) 

```
[Unit]
Description=nameOfService

[Service]
ExecStart=/path/to/executable

[Install]
WantedBy=multi-user.target
```

Dbus configuration
------------------

Typical default D-Bus configuration does not allow to register services except explicitly allowed. Filename should 
contain name of your service, e.g `/etc/dbus-1/system.d/org.sdbuscpp.concatenator.conf`. So, here is template
configuration to use dbus interface under root:

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

If you need access from other user `root` should be substituted by desired username. For more refer to `man dbus-daemon`.