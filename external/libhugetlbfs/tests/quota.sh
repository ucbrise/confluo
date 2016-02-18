#!/bin/bash

. wrapper-utils.sh

# There are known bugs in quota accounting prior to 2.6.24
compare_kvers `uname -r` "2.6.24"
if [ $? -eq 1 ]; then
	EXP_RC=$RC_FAIL
else
	EXP_RC=$RC_PASS
fi

exec_and_check $EXP_RC quota "$@"
