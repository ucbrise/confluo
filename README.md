# DiaLog

[![Build Status](https://amplab.cs.berkeley.edu/jenkins/job/dialog/badge/icon)](https://amplab.cs.berkeley.edu/jenkins/job/dialog/)
[![License](http://img.shields.io/:license-Apache%202-red.svg)](LICENSE)

DiaLog is a system for real-time monitoring and analysis of data, that supports:
* Highly concurrent, lock-free reads and writes per server
* Support for _online_ and _offline_ analysis of data
* Useful system properties: Atomicity, Distributed Snapshots, Durability,
  Archival, etc.
