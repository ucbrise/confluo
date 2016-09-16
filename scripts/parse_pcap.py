#!/usr/bin/env python

import dpkt
import sys
import socket

def main(f):
  pcap = dpkt.pcap.Reader(f)
  for ts, buf in pcap:
    eth = dpkt.ethernet.Ethernet(buf)
    if eth.data.__class__.__name__ == 'IP' and (eth.data.data.__class__.__name__ == 'TCP' or eth.data.data.__class__.__name__ == 'UDP'):
      attrs = (ts, eth.data.src, eth.data.dst, eth.data.data.sport, eth.data.data.dport)
      print attrs
    else:
      attrs = (ts)
      print attrs

if __name__ == "__main__" :
  f = open(sys.argv[1])
  main(f)