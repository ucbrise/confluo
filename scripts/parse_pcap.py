#!/usr/bin/env python

import dpkt
import sys
import socket

def main(f):
  pcap = dpkt.pcap.Reader(f)
  for ts, buf in pcap:
    eth = dpkt.ethernet.Ethernet(buf)
    print "%d: %s" % (ts, eth.data.__name__)  

if __name__ == "__main__" :
  f = open(sys.argv[1])
  main(f)