#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import socket
import selectors
import types
#################################################################

selektorius = selectors.DefaultSelector()   # bus parinktas tinkamiausias FD valdymas (select(), poll(), epoll(), ..) 
klientas = 0
#################################################################

def prijungimas(soketas):
    jungtis, adresas = soketas.accept()  # 
    global klientas
    klientas += 1
    print("Priimtas prisijungimas iš ", adresas, ', klientas: ', klientas)
    jungtis.setblocking(False)
    duomenys = types.SimpleNamespace(adresas=adresas, inb=b"", outb=b"", klientas=klientas)
    ivykiai = selectors.EVENT_READ | selectors.EVENT_WRITE
    selektorius.register(jungtis, ivykiai, data=duomenys)

#################################################################

def aptarnaukPrisijungima(key, mask):
    global klientas
    soketas = key.fileobj
    duomenys = key.data
    if mask & selectors.EVENT_READ:
        gautiDuomenys = soketas.recv(1024)  # 
        if gautiDuomenys:
            duomenys.outb += gautiDuomenys
        else:
            print("Uždarome: ", duomenys.adresas)
            selektorius.unregister(soketas)
            soketas.close()
    if mask & selectors.EVENT_WRITE:
        if duomenys.outb:
            print("Siunčiame pakeistą eilutę ", repr(duomenys.outb), " --> ", duomenys.adresas)
            issiusta = soketas.send(duomenys.outb.upper())  
            duomenys.outb = duomenys.outb[issiusta:]

#################################################################

hostas = '127.0.0.1'
portas = 10000


klsok = socket.socket(socket.AF_INET, socket.SOCK_STREAM)    # Klausymo soketas
klsok.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)  #!!!!!!!!!!!!!!!!! pavojinga !!!!!!!!!!!!!

klsok.bind((hostas, portas))
klsok.listen()
print("Bandome klausyti ...", (hostas, portas))
klsok.setblocking(False)


selektorius.register(klsok, selectors.EVENT_READ, data=None)

try:
    while True:
        ivykiai = selektorius.select()
        for key, mask in ivykiai:
            if key.data is None:
                prijungimas(key.fileobj)
            else:
                aptarnaukPrisijungima(key, mask)
except KeyboardInterrupt:
    print("Nutraukta su klaviatūra ...")
finally:
    selektorius.close()
    klsok.close()
