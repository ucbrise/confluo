# DiaLog

[![Build Status](https://amplab.cs.berkeley.edu/jenkins/job/dialog/badge/icon)](https://amplab.cs.berkeley.edu/jenkins/job/dialog/)
[![License](http://img.shields.io/:license-Apache%202-red.svg)](LICENSE)

DiaLog is a system for real-time monitoring and analysis of data, that supports:
* Highly concurrent, lock-free reads and writes per server
* Support for _online_ and _offline_ analysis of data
* Useful system properties: Atomicity, Distributed Snapshots, Durability,
  Archival, etc.

## Installing from source

### Requirements

* C++ compiler that supports C++11 standard
* CMake 3.2 or later
* Boost 1.55 or later

For python client, you will additionally require:
* Python 2.7 or later
* Python Packages: six 1.7.2 or later

### Installing

DiaLog only supports out of source builds:

```bash
mkdir build
cd build
cmake ..
make -j && make test && make install
```
