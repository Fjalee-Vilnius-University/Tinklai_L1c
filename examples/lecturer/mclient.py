#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import sys
import socket
import selectors
import types
##############################################################################
selektorius = selectors.DefaultSelector()
pranesimai = [bytearray('Labas 1','utf-8'), bytearray('Labas 2','utf-8')]
##############################################################################

def bandome(hostas, portas, kiekPrisijungimu):
    serverioAdresas = (hostas, portas)
    for i in range(0, kiekPrisijungimu):
        jungtiesID = i + 1
        print("Pradedame prisijungti su ID ", jungtiesID, " prie ", serverioAdresas)
        soketas = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        soketas.setblocking(False)
        soketas.connect_ex(serverioAdresas)
        ivykiai = selectors.EVENT_READ | selectors.EVENT_WRITE
        duomenys = types.SimpleNamespace(
            jungtiesID=jungtiesID,
            msg_total=sum(len(m) for m in pranesimai),
            recv_total=0,
            pranesimai=list(pranesimai),
            outb=b"",
        )
        selektorius.register(soketas, ivykiai, data=duomenys)

##############################################################################

def aptarnaukPrisijungima(key, mask):
    soketas = key.fileobj
    duomenys = key.data
    if mask & selectors.EVENT_READ:
        recv_data = soketas.recv(1024)  # 
        if recv_data:
            print("Gavome ", repr(recv_data), " iš ", duomenys.jungtiesID)
            duomenys.recv_total += len(recv_data)
        if not recv_data or duomenys.recv_total == duomenys.msg_total:
            print("Uždarome prisijungimą ", duomenys.jungtiesID)
            selektorius.unregister(soketas)
            soketas.close()
    if mask & selectors.EVENT_WRITE:
        if not duomenys.outb and duomenys.pranesimai:
            duomenys.outb = duomenys.pranesimai.pop(0)
        if duomenys.outb:
            print("siunčiame", repr(duomenys.outb), " prisijungimui", duomenys.jungtiesID)
            sent = soketas.send(duomenys.outb)  # 
            duomenys.outb = duomenys.outb[sent:]


#################################################################

hostas = '127.0.0.1'
portas = 10000
kiekPrisijungimu = 1 

bandome(hostas, portas, kiekPrisijungimu)
 
try:
    while True:
        ivykiai = selektorius.select(timeout=1)
        if ivykiai:
            for key, mask in ivykiai:
                aptarnaukPrisijungima(key, mask)
        # Check for a socket being monitored to continue.
        if not selektorius.get_map():
            break
except KeyboardInterrupt:
    print("Nutraukta su klaviatūra ...")
finally:
    selektorius.close()
