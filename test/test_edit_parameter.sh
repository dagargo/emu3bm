#!/usr/bin/env bash

. $srcdir/test_common.sh

TEST_BANK_NAME=$srcdir/test_add_zone

cleanUp

logAndRun '$srcdir/../src/emu3bm -n $TEST_BANK_NAME'
test

logAndRun '$srcdir/../src/emu3bm -p "P0" $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME data/test_add_zone_1'
test

logAndRun '$srcdir/../src/emu3bm -s data/s1.wav $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME data/test_add_zone_2'
test

logAndRun '$srcdir/../src/emu3bm -e 0 -Z 1,pri,20,15,26 $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME data/test_add_zone_3'
test

logAndRun '$srcdir/../src/emu3bm -e 0 -q 25 -c 200 $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME data/test_edit_parameter_1'
test

cleanUp
