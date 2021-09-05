#!/usr/bin/env bash

. $srcdir/test_common.sh

TEST_BANK_NAME=$srcdir/test_add_sample

cleanUp

logAndRun '$srcdir/../src/emu3bm -n $TEST_BANK_NAME'
test

logAndRun '$srcdir/../src/emu3bm -s foo $TEST_BANK_NAME'
testError

logAndRun '$srcdir/../src/emu3bm -s data/s1.wav $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME data/test_add_sample_1'
test

logAndRun '$srcdir/../src/emu3bm -S data/s1.wav $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME data/test_add_sample_2'
test

logAndRun '$srcdir/../src/emu3bm -s data/s2.wav $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME data/test_add_sample_3'
test

logAndRun '$srcdir/../src/emu3bm -S data/s2.wav $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME data/test_add_sample_4'
test

cleanUp
