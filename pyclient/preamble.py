#!/usr/bin/env python

import atexit
import os
import readline
import rlcompleter
import sys
from thrift import Thrift

from confluo.rpc import client

confluo = None


def confluo_connect(host='localhost', port=9090):
    global confluo
    confluo = client.RpcClient(host, port)


try:
    confluo_connect()
    print "Confluo Client is now available as confluo."
except Thrift.TException, tx:
    print tx.message
    print 'Check your server status and retry connecting with confluo_connect(host, port)'

# Add auto-completion and a stored history file of commands to your Python
# interactive interpreter. Requires Python 2.0+, readline. Autocomplete is
# bound to the Esc key by default.
#
# Store the file in ~/.pystartup, and set an environment variable to point
# to it:  "export PYTHONSTARTUP=~/.pystartup" in bash.


historyPath = os.path.expanduser("~/.pyhistory")


def save_history(path=historyPath):
    import readline
    readline.write_history_file(path)


if os.path.exists(historyPath):
    readline.read_history_file(historyPath)

atexit.register(save_history)
readline.parse_and_bind('tab: complete')

del os, atexit, readline, rlcompleter, save_history, historyPath, sys
