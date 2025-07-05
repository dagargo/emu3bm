#!/usr/bin/env bash

. $srcdir/test_common.sh

TEST_BANK_NAME=$srcdir/emu3_test_create_bank

cleanUp

logAndRun '$srcdir/../src/emu3bm -n $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME data/emu3_test_create_bank_esi2000'
test

cleanUp

logAndRun '$srcdir/../src/emu3bm -d esi2000 -n $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME data/emu3_test_create_bank_esi2000'
test

cleanUp

logAndRun '$srcdir/../src/emu3bm -d emu3x -n $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME data/emu3_test_create_bank_emu3x'
test

cleanUp
