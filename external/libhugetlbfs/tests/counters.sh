#!/bin/bash

. wrapper-utils.sh

# Huge page overcommit was not available until 2.6.24
compare_kvers `uname -r` "2.6.24"
if [ $? -eq 1 ]; then
	EXP_RC=$RC_FAIL
else
	EXP_RC=$RC_PASS
fi

exec_and_check $EXP_RC counters "$@"
