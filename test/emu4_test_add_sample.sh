#!/usr/bin/env bash

. $SRCDIR/test_common.sh

TEST_BANK_NAME=$(mktemp -d)/emu4_test_add_sample

logAndRun '$BINDIR/emu4bm -n $TEST_BANK_NAME'
test

logAndRun '$BINDIR/emu4bm -s $SRCDIR/data/s1_loop.wav $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME $SRCDIR/data/emu4_test_add_sample_1'
test

logAndRun '$BINDIR/emu4bm -s $SRCDIR/data/s2_loop.wav $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME $SRCDIR/data/emu4_test_add_sample_2'
test

logAndRun '$BINDIR/emu4bm -n $TEST_BANK_NAME'
test
for s in $(seq 1 999); do
	echo "Adding sample..."
	logAndRun '$BINDIR/emu4bm -s $SRCDIR/data/s1.wav $TEST_BANK_NAME' > /dev/null
	test
done
logAndRun '$BINDIR/emu4bm -s $SRCDIR/data/s1.wav $TEST_BANK_NAME'
testError

cleanUp
