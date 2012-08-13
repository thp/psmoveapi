#!/usr/bin/python
# -*- coding: utf-8 -*-
#
# PS Move API - An interface for the PS Move Motion Controller
# Copyright (c) 2012 Thomas Perl <m@thp.io>
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#    1. Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#
#    2. Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

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

