#!/usr/bin/env python

import dpkt
import sys
import socket

def main(f):
  pcap = dpkt.pcap.Reader(f)
  wbytes = 0
  for ts, buf in pcap:
    eth = dpkt.ethernet.Ethernet(buf)
    attrs = (ts, wbytes, 0, 0, 0, 0)
    if eth.type == dpkt.ethernet.ETH_TYPE_IP:
      ip = eth.data
      if ip.p == dpkt.ip.IP_PROTO_TCP:
        tcp = ip.data
        attrs = (ts, wbytes, ip.src, ip.dst, tcp.src, tcp.dst)
    wbytes += len(buf)
    print attrs

if __name__ == "__main__" :
  inp = open(sys.argv[1])
  outd = open(sys.argv[2] + ".data", 'wb')
  outa = open(sys.argv[3] + ".attr", 'wb')
  main(f)