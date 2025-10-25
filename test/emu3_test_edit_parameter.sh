#!/usr/bin/env bash

. $srcdir/test_common.sh

TEST_BANK_NAME=$srcdir/emu3_test_edit_parameter

cleanUp

cp data/emu3_test_add_zone_3 $TEST_BANK_NAME

logAndRun '$srcdir/../src/emu3bm -e 0 -q 25 -c 200 $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME data/emu3_test_edit_parameter_1'
test

cleanUp
