#!/usr/bin/python
# -*- coding: utf-8 -*-
#
# create_move.py - Create device entry for a Move Motion Controller
# 2012-06-02 Thomas Perl <thp.io/about>
#

import dbus
import sys

if len(sys.argv) != 2:
    print >>sys.stderr, """
    Usage: %s <btaddr>

    Tries to add the controller with address <btaddr> to the
    bluetoothd list of known devices via D-Bus.
    """ % sys.argv[0]
    sys.exit(1)

address = sys.argv[1]

system_bus = dbus.SystemBus()

manager_object = system_bus.get_object('org.bluez', '/')
manager = dbus.Interface(manager_object, 'org.bluez.Manager')

adapter_path = manager.DefaultAdapter()
adapter_object = system_bus.get_object('org.bluez', adapter_path)
adapter = dbus.Interface(adapter_object, 'org.bluez.Adapter')

try:
    adapter.CreateDevice(address)
except Exception, e:
    print 'CreateDevice exception:', e

for device in adapter.ListDevices():
    device_object = system_bus.get_object('org.bluez', device)
    device = dbus.Interface(device_object, 'org.bluez.Device')
    properties = device.GetProperties()
    if properties['Address'].lower() == address.lower():
        print 'Setting device as trusted...'
        device.SetProperty('Trusted', True)
        break

