#!/usr/bin/env python
# -*- coding: UTF-8 -*-
try:
    import scapy.all as scapy
except ImportError:
    import scapy
    

def main(inp, outd, outa):
  packets = scapy.rdpcap(inp)
  wbytes = 0
  for packet in packets:
    attrs = (packet.time, wbytes, 0, 0, 0, 0)
    if IP in packet and TCP in packet:
      attrs = (packet.time, wbytes, pkt[IP].src, pkt[IP].dst, pkt[TCP].sport, pkt[TCP].dport)
    wbytes += len(buf)
    print attrs

if __name__ == "__main__" :
  inp = sys.argv[1]
  outd = open(sys.argv[2] + ".data", 'wb')
  outa = open(sys.argv[2] + ".attr", 'wb')
  main(inp, outd, outa)