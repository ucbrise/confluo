#!/usr/bin/env python

import sys

def compute_size(size, block_size):
  while (block_size < size):
    block_size *= 2
  return float(block_size)

def main(num_records):
  fixed_overhead = 403701760
  dlog_data = 54 * num_records
  dlog_size = compute_size(dlog_data, 67108864)
  olog_size = compute_size(num_records, 32) * 16.0
  ilog_size = compute_size(num_records, 32) * 8.0 * 5.0

  print (fixed_overhead + dlog_size + olog_size + ilog_size) / (1024.0*1024.0*1024.0)
  
if __name__ == "__main__" :
  num_records = int(sys.argv[1])
  main(num_records)