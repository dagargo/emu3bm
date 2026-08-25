#!/usr/bin/env bash

. $SRCDIR/test_common.sh

TEST_BANK_NAME=$(mktemp -d)/emu3_test_add_sample

logAndRun '$BINDIR/emu3bm -n $TEST_BANK_NAME'
test

logAndRun '$BINDIR/emu3bm -s foo $TEST_BANK_NAME'
testError

# No loop info (no "smpl" chunk)
logAndRun '$BINDIR/emu3bm -s $SRCDIR/data/s1.wav $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME $SRCDIR/data/emu3_test_add_sample_1'
test

logAndRun '$BINDIR/emu3bm -s $SRCDIR/data/s2.wav $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME $SRCDIR/data/emu3_test_add_sample_2'
test

# Loop info ("smpl" chunk with loop type 0)
logAndRun '$BINDIR/emu3bm -s $SRCDIR/data/s1_loop.wav $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME $SRCDIR/data/emu3_test_add_sample_3'
test

# Loop info ("smpl" chunk with loop type 0x7f, AKA no-loop)
logAndRun '$BINDIR/emu3bm -s $SRCDIR/data/s2_loop.wav $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME $SRCDIR/data/emu3_test_add_sample_4'
test

logAndRun '$BINDIR/emu3bm -n $TEST_BANK_NAME'
test
for s in $(seq 1 999); do
	logAndRun '$BINDIR/emu3bm -s $SRCDIR/data/s1.wav $TEST_BANK_NAME'
	test
done
logAndRun '$BINDIR/emu3bm -s $SRCDIR/data/s1.wav $TEST_BANK_NAME'
testError

cleanUp
