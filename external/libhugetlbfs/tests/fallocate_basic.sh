#!/bin/bash

. wrapper-utils.sh

#
# hugetlbfs fallocate support was not available until 4.3
#
compare_kvers `uname -r` "4.3.0"
if [ $? -eq 1 ]; then
	echo "FAIL no fallocate support in kernels before 4.3.0"
	exit $RC_FAIL
else
	EXP_RC=$RC_PASS
	exec_and_check $EXP_RC fallocate_basic "$@"
fi
