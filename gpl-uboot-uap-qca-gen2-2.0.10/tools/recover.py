#!/usr/bin/env python

import socket
from time import sleep
import time

recovery_port = 10002
beacon_port = 10001

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
sock.bind(('', beacon_port))
sock.setblocking(0)

discovered = {}

while True:
    rxing = True
    while rxing:
        try:
            (data, addr) = sock.recvfrom(512)
            if data[0] != "\x03" or data[1] != "\x07": # UAP uboot beacon
                continue

            ip = addr[0]
            if ip not in discovered:
                discovered[ip] = { 'last_announced': time.time() }
                print('Found %s' % ip)
            else:
                disc = discovered[ip]
                if time.time() - disc['last_announced'] >= 60:
                    print('Found %s' % ip)
                    disc['last_announced'] = time.time()
        except socket.error:
            rxing = False

    # 172.16.x.x by default
    # Magic, version, cmd (recovery), reserved(2), ipaddr, netmask
    sock.sendto("\x8e\x46\x93\x78" "\x01" "\x01" "\x00\x00" "\xac\x10\x00\x00" "\xff\xff\x00\x00", ("255.255.255.255", recovery_port))
    sleep(0.5)
