#!/usr/bin/env bash

. $srcdir/test_common.sh

TEST_BANK_NAME=$srcdir/emu3_test_add_sample

cleanUp

logAndRun '$srcdir/../src/emu3bm -n $TEST_BANK_NAME'
test

logAndRun '$srcdir/../src/emu3bm -s foo $TEST_BANK_NAME'
testError

# No loop info (no "smpl" chunk)
logAndRun '$srcdir/../src/emu3bm -s data/s1.wav $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME data/emu3_test_add_sample_1'
test

logAndRun '$srcdir/../src/emu3bm -s data/s2.wav $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME data/emu3_test_add_sample_2'
test

# Loop info ("smpl" chunk with loop type 0)
logAndRun '$srcdir/../src/emu3bm -s data/s1_loop.wav $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME data/emu3_test_add_sample_3'
test

# Loop info ("smpl" chunk with loop type 0x7f, AKA no-loop)
logAndRun '$srcdir/../src/emu3bm -s data/s2_loop.wav $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME data/emu3_test_add_sample_4'
test

logAndRun '$srcdir/../src/emu3bm -n $TEST_BANK_NAME'
test
for s in $(seq 1 999); do
	logAndRun '$srcdir/../src/emu3bm -s data/s1.wav $TEST_BANK_NAME'
	test
done
logAndRun '$srcdir/../src/emu3bm -s data/s1.wav $TEST_BANK_NAME'
testError

cleanUp
