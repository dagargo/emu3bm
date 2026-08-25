#!/usr/bin/env bash

. $SRCDIR/test_common.sh

TEST_BANK_NAME=$(mktemp -d)/emu3_test_add_preset

logAndRun '$BINDIR/emu3bm -n $TEST_BANK_NAME'
test

logAndRun '$BINDIR/emu3bm -p "Preset 0" $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME $SRCDIR/data/emu3_test_add_preset_1'
test

logAndRun '$BINDIR/emu3bm -p "Preset 1" $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME $SRCDIR/data/emu3_test_add_preset_2'
test

logAndRun '$BINDIR/emu3bm -n $TEST_BANK_NAME'
test
for s in $(seq 0 255); do
	logAndRun '$BINDIR/emu3bm -p "Preset x" $TEST_BANK_NAME'
	test
done
logAndRun '$BINDIR/emu3bm -p "Preset x" $TEST_BANK_NAME'
testError

cleanUp
