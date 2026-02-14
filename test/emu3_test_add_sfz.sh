#!/usr/bin/env bash

. $srcdir/test_common.sh

TEST_BANK_NAME=$srcdir/emu3_test_add_sfz

cleanUp

logAndRun '$srcdir/../src/emu3bm -n $TEST_BANK_NAME'
logAndRun '$srcdir/../src/emu3bm -S data/test1.sfz $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME data/emu3_test_add_sfz_1'
test

logAndRun '$srcdir/../src/emu3bm -n $TEST_BANK_NAME'
logAndRun '$srcdir/../src/emu3bm -S data/test2.sfz $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME data/emu3_test_add_sfz_2'
test

logAndRun '$srcdir/../src/emu3bm -n $TEST_BANK_NAME'
logAndRun '$srcdir/../src/emu3bm -S data/test3.sfz $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME data/emu3_test_add_sfz_3'
test

logAndRun '$srcdir/../src/emu3bm -n $TEST_BANK_NAME'
logAndRun '$srcdir/../src/emu3bm -S data/test4.sfz $TEST_BANK_NAME'
testError

logAndRun '$srcdir/../src/emu3bm -n $TEST_BANK_NAME'
logAndRun '$srcdir/../src/emu3bm -S data/test5.sfz $TEST_BANK_NAME'
testError

logAndRun '$srcdir/../src/emu3bm -n $TEST_BANK_NAME'
logAndRun '$srcdir/../src/emu3bm -S data/test6.sfz $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME data/emu3_test_add_sfz_6'
test

logAndRun '$srcdir/../src/emu3bm -n $TEST_BANK_NAME'
logAndRun '$srcdir/../src/emu3bm -S data/test7.sfz $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME data/emu3_test_add_sfz_7'
test

logAndRun '$srcdir/../src/emu3bm -n $TEST_BANK_NAME'
logAndRun '$srcdir/../src/emu3bm -S data/test8.sfz $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME data/emu3_test_add_sfz_8'
test

rm $TEST_BANK_NAME_PRISTINE

cleanUp
