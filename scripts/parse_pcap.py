#!/usr/bin/env python

import dpkt
import sys
import socket

def main(inp, outd, outa):
  pcap = dpkt.pcap.Reader(inp)
  wbytes = 0
  for ts, buf in pcap:
    outd.write(buf)
    wbytes = len(buf)
    eth = dpkt.ethernet.Ethernet(buf)
    attrs = (long(ts*1000000L), wbytes, 0, 0, 0, 0)
    if eth.type == dpkt.ethernet.ETH_TYPE_IP:
      ip = eth.data
      if ip.p == dpkt.ip.IP_PROTO_TCP:
        tcp = ip.data
        attrs = (long(ts*1000000L), wbytes, socket.inet_ntoa(ip.src), socket.inet_ntoa(ip.dst), tcp.sport, tcp.dport)
    print "%d %d %s %s %d %d" % attrs

if __name__ == "__main__" :
  inp = open(sys.argv[1])
  outd = open(sys.argv[2] + ".data", 'wb')
  outa = open(sys.argv[2] + ".attr", 'wb')
  main(inp, outd, outa)