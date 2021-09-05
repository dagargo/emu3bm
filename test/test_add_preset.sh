#!/usr/bin/env bash

. $srcdir/test_common.sh

TEST_BANK_NAME=$srcdir/test_add_preset

cleanUp

logAndRun '$srcdir/../src/emu3bm -n $TEST_BANK_NAME'
test

logAndRun '$srcdir/../src/emu3bm -p "Preset 0" $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME data/test_add_preset_1'
test

logAndRun '$srcdir/../src/emu3bm -p "Preset 1" $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME data/test_add_preset_2'
test

cleanUp
