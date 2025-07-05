#!/usr/bin/env bash

. $srcdir/test_common.sh

TEST_BANK_NAME=$srcdir/emu4_test_add_sample

cleanUp

logAndRun '$srcdir/../src/emu4bm -n $TEST_BANK_NAME'
test

logAndRun '$srcdir/../src/emu4bm -s data/s1_loop.wav $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME data/emu4_test_add_sample_1'
test

logAndRun '$srcdir/../src/emu4bm -s data/s2_loop.wav $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME data/emu4_test_add_sample_2'
test

logAndRun '$srcdir/../src/emu4bm -n $TEST_BANK_NAME'
test
for s in $(seq 1 999); do
	logAndRun '$srcdir/../src/emu4bm -s data/s1.wav $TEST_BANK_NAME'
	test
done
logAndRun '$srcdir/../src/emu4bm -s data/s1.wav $TEST_BANK_NAME'
testError

cleanUp
