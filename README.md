# Confluo

[![Build Status](https://amplab.cs.berkeley.edu/jenkins/job/confluo/badge/icon)](https://amplab.cs.berkeley.edu/jenkins/job/confluo/)
[![License](http://img.shields.io/:license-Apache%202-red.svg)](LICENSE)

Confluo is a system for real-time monitoring and analysis of data, that supports:
* Highly concurrent, lock-free reads and writes per server
* Support for _online_ and _offline_ analysis of data
* Useful system properties: Atomicity, Distributed Snapshots, Durability,
  Archival, etc.

## Installing from source

### Requirements

* C++ compiler that supports C++11 standard
* CMake 2.8 or later
* Boost 1.53 or later

For python client, you will additionally require:
* Python 2.7 or later
* Python Packages: six 1.7.2 or later

For java client, you will additionally require:
* Java 1.7 or later
* ant 1.6.2 or later

### Installing

Confluo only supports out of source builds:

```bash
mkdir build
cd build
cmake ..
make -j && make test && make install
```
