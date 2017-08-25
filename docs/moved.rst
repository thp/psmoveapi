Networking (Move Daemon)
========================

PS Move API contains a feature that allows you to connect more than 7
PS Move controllers to a single instance, even on systems that do not
allow multiple Bluetooth adapters. The way this works is by exposing
the connected controllers via IP (UDP).

This feature has been used by e.g. `Edgar Rice Soiree`_ to run a game
with 20 move controllers distributed over 3 PCs.

.. _`Edgar Rice Soiree`: http://thp.io/2012/tarzan/

moved2_hosts.txt
----------------

You need to place a file "moved2_hosts.txt" into the data directory of PS Move
API (usually ~/.psmoveapi/, in Windows %APPDATA%/.psmoveapi/) with one IP
address per line that points to a moved instance.

Example moved2_hosts.txt contents::

    192.168.0.2

This would try to use the moved running on 192.168.0.2. You can have multiple
IPs listed there (one per line), the servers will be tried in order.

If you add * on a single line, this will enable local network auto-discovery,
meaning that any running Move Daemon instances can be auto-discovered via UDP
broadcast.


API Usage
---------

After that, you can use the PS Move API just as you normally would, and after
your locally-connected controllers, controllers connected to remote machines
will be used (psmove_count_connected() and psmove_connect_by_id() will take
the remote controllers into account).

If you want to only use remotely-connected controllers (ignoring any
controllers that are connected locally), or if you want to use moved running
on localhost (where it will already provide the connected controllers to you),
you should add the following API call at the beginning of your application to
ensure that locally-connected devices are not used via hidapi::

    psmove_set_remote_config(PSMove_OnlyRemote);

Similarly, you can disable remote controllers via the following API call (in
that case, no connections to moved are carred out, only hidapi is used)::

    psmove_set_remote_config(PSMove_OnlyLocal);

