# -*- coding: utf-8 -*-
#
# psmoveplug.py - Wrapper script for moved, displaying on LCD2USB
# Thomas Perl <m@thp.io>; 2012-09-13 / 2012-09-14
#

import subprocess
import re


class Display:
    LCD2USB = '/usr/local/bin/lcd2usb'
    IFCONFIG = '/sbin/ifconfig'

    def __init__(self):
        self.proc = subprocess.Popen([self.LCD2USB], stdin=subprocess.PIPE)

    def send(self, lines):
        # shuffle + pad lines to fit my lcd2usb setup
        if len(lines) < 4:
            lines += [' '] * (4 - len(lines))
        lines = ['', ''] + lines[::2] + lines[1::2]

        for line in lines:
            self.proc.stdin.write(line + '\n')

    def get_ip(self):
        # based on code from http://thp.io/2011/ipaddr-widget/
        ifconfig = subprocess.Popen([self.IFCONFIG], stdout=subprocess.PIPE)
        stdout, _ = ifconfig.communicate()

        ips = re.findall('addr:([^ ]+)', stdout)
        result = filter(lambda ip: not ip.startswith('127.'), ips)

        if result:
            return result[0]
        else:
            return 'offline'

    def update(self, message):
        self.send([
            'PS Move Plug',
            ' ',
            'IP: %s' % self.get_ip(),
            'moved: %s' % message,
        ])


class MoveDaemon:
    MOVED = '/usr/bin/moved'

    def __init__(self, display):
        self.display = display

    def run(self):
        proc = subprocess.Popen([self.MOVED], stdout=subprocess.PIPE)
        while True:
            self.display.update(proc.stdout.readline().strip())


display = Display()
moved = MoveDaemon(display)
moved.run()

