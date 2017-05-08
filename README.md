# DiaLog

[![Build Status](https://amplab.cs.berkeley.edu/jenkins/job/dialog/badge/icon)](https://amplab.cs.berkeley.edu/jenkins/job/dialog/)
[![License](http://img.shields.io/:license-Apache%202-red.svg)](LICENSE)

DiaLog is a distributed system that achieves the three properties useful for storage and real-time analysis of machine-generated data: 
1. support for storing raw data along with several aggregate statistics at each server; 
2. millions on concurrent reads and writes at each server; and 
3. strong consistency semantics for data as well as secondary data structures.
