#!/bin/bash

. wrapper-utils.sh

# fadvise is known broken before 2.6.30
compare_kvers `uname -r` "2.6.30"
if [ $? -eq 1 ]; then
	echo "FAIL (assumed) kernel bug"
	exit $RC_FAIL
else
	EXP_RC=$RC_PASS
	exec_and_check $EXP_RC fadvise_reserve "$@"
fi

