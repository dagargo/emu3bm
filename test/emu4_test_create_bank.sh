#!/usr/bin/env bash

. $srcdir/test_common.sh

TEST_BANK_NAME=$srcdir/emu4_test_create_bank

cleanUp

logAndRun '$srcdir/../src/emu4bm -n $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME data/emu4_test_create_bank'
test

cleanUp
