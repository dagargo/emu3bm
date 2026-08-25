#!/usr/bin/env bash

. $SRCDIR/test_common.sh

TEST_BANK_NAME=$(mktemp -d)/emu3_test_add_sfz

logAndRun '$BINDIR/emu3bm -n $TEST_BANK_NAME'
logAndRun '$BINDIR/emu3bm -S $SRCDIR/data/test1.sfz $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME $SRCDIR/data/emu3_test_add_sfz_1'
test

logAndRun '$BINDIR/emu3bm -n $TEST_BANK_NAME'
logAndRun '$BINDIR/emu3bm -S $SRCDIR/data/test2.sfz $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME $SRCDIR/data/emu3_test_add_sfz_2'
test

logAndRun '$BINDIR/emu3bm -n $TEST_BANK_NAME'
logAndRun '$BINDIR/emu3bm -S $SRCDIR/data/test3.sfz $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME $SRCDIR/data/emu3_test_add_sfz_3'
test

logAndRun '$BINDIR/emu3bm -n $TEST_BANK_NAME'
logAndRun '$BINDIR/emu3bm -S $SRCDIR/data/test4.sfz $TEST_BANK_NAME'
testError

logAndRun '$BINDIR/emu3bm -n $TEST_BANK_NAME'
logAndRun '$BINDIR/emu3bm -S $SRCDIR/data/test5.sfz $TEST_BANK_NAME'
testError

logAndRun '$BINDIR/emu3bm -n $TEST_BANK_NAME'
logAndRun '$BINDIR/emu3bm -S $SRCDIR/data/test6.sfz $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME $SRCDIR/data/emu3_test_add_sfz_6'
test

logAndRun '$BINDIR/emu3bm -n $TEST_BANK_NAME'
logAndRun '$BINDIR/emu3bm -S $SRCDIR/data/test7.sfz $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME $SRCDIR/data/emu3_test_add_sfz_7'
test

logAndRun '$BINDIR/emu3bm -n $TEST_BANK_NAME'
logAndRun '$BINDIR/emu3bm -S $SRCDIR/data/test8.sfz $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME $SRCDIR/data/emu3_test_add_sfz_8'
test

logAndRun '$BINDIR/emu3bm -n $TEST_BANK_NAME'
logAndRun '$BINDIR/emu3bm -S $SRCDIR/data/test9.sfz $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME $SRCDIR/data/emu3_test_add_sfz_9'
test

cleanUp
