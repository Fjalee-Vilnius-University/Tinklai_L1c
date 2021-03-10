#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Echo serveris 
import socket
import sys

hostas = '127.0.0.1'                                            # Lokalus  IPv4
#hostas = '::1'                                            # Lokalus  IPv6#
#portas = int(input('Įvesk porto numerį (>1024) --> '))          # Bet kuris „tinkamas“ portas  
portas = 20000
s = None
for rezultatas in socket.getaddrinfo(hostas, portas, socket.AF_UNSPEC,
                              socket.SOCK_STREAM, 0, socket.AI_PASSIVE):
    adresu_seima, soketo_tipas, protokolas, canoninis_vardas, soketo_adresas = rezultatas
    print('Rezultatas ' + str(rezultatas))

    try:
        s = socket.socket(adresu_seima, soketo_tipas, protokolas)
    except OSError as msg:
        s = None
        continue
    try:
        s.bind(soketo_adresas)
        s.listen(1)
    except OSError as msg:
        s.close()
        s = None
        continue
    break
if s is None:
    print('Negaliu atverti soketo')
    sys.exit(1)
jungtis, adresas = s.accept()
jungtis.setblocking(False)
ciklas = 0
while 1:
    ciklas += 1
    duomenys = None
    try:
        duomenys = jungtis.recv(1024)
    except BlockingIOError as msg:
        if ciklas % 1000000 == 0:
           print('Laukiame ...')
    if duomenys: 
        print(duomenys)
        did_duomenys = duomenys.upper()   
        jungtis.send(did_duomenys)
jungtis.close()









