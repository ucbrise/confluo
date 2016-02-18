#!/bin/bash

# Standard return codes
RC_PASS=0
RC_CONFIG=1
RC_FAIL=2
RC_XFAIL=3
RC_XPASS=4
RC_BUG=99

function unexpected_pass()
{
	echo -n "UNEXPECTED "
}

function expected_fail()
{
	echo -n "EXPECTED "
}

# check_rc (<expected return code>, <actual return code>)
# Returns: Adjusted return code
#
# Check the actual and expected return codes to identify
# expected failures and unexpected passes.
function check_rc()
{
	EXP_RC=$1
	ACT_RC=$2

	if [ $ACT_RC -eq $RC_PASS -a $EXP_RC -ne $RC_PASS ]; then
		unexpected_pass
		return $RC_XPASS
	elif [ $EXP_RC -ne $RC_PASS -a $EXP_RC -eq $ACT_RC ]; then
		expected_fail
		return $RC_XFAIL
	else
		return $ACT_RC
	fi
}

# exec_and_check (<expected return code>, <command-line ...>)
# Does not return
# Execute a test command and check for expected failures and unexpected passes.
function exec_and_check()
{
	EXP_RC=$1
	shift

	OUTPUT=`$@`
	check_rc $EXP_RC $?
	RC=$?
	echo $OUTPUT

	exit $RC
}
