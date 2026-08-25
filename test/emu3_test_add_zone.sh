#!/usr/bin/env bash

. $SRCDIR/test_common.sh

TEST_BANK_NAME=$(mktemp -d)/emu3_test_add_zone

logAndRun '$BINDIR/emu3bm -n $TEST_BANK_NAME'
test

logAndRun '$BINDIR/emu3bm -e 0 -z 1,pri,F1,C1,B1 $TEST_BANK_NAME'
testError

logAndRun '$BINDIR/emu3bm -p "P0" $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME $SRCDIR/data/emu3_test_add_zone_1'
test

logAndRun '$BINDIR/emu3bm -s $SRCDIR/data/s1.wav $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME $SRCDIR/data/emu3_test_add_zone_2'
test

logAndRun '$BINDIR/emu3bm -e 0 -z 1,pri,F1,C1,B1 $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME $SRCDIR/data/emu3_test_add_zone_3'
test

logAndRun '$BINDIR/emu3bm -e 0 -z 1,pri,F2,C2,B2 $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME $SRCDIR/data/emu3_test_add_zone_4'
test

logAndRun '$BINDIR/emu3bm -e 0 -z 1,sec,f1,c1,b1 $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME $SRCDIR/data/emu3_test_add_zone_5'
test

logAndRun '$BINDIR/emu3bm -n $TEST_BANK_NAME'
test

logAndRun '$BINDIR/emu3bm -p "P0" $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME $SRCDIR/data/emu3_test_add_zone_1'
test

logAndRun '$BINDIR/emu3bm -s $SRCDIR/data/s1.wav $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME $SRCDIR/data/emu3_test_add_zone_2'
test

logAndRun '$BINDIR/emu3bm -e 0 -Z 1,pri,20,15,26 $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME $SRCDIR/data/emu3_test_add_zone_3'
test

logAndRun '$BINDIR/emu3bm -e 0 -Z 1,pri,32,27,38 $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME $SRCDIR/data/emu3_test_add_zone_4'
test

logAndRun '$BINDIR/emu3bm -e 0 -Z 1,sec,20,15,26 $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME $SRCDIR/data/emu3_test_add_zone_5'
test

logAndRun '$BINDIR/emu3bm -e 0 --delete-zone 0 $TEST_BANK_NAME'
test
logAndRun '$BINDIR/emu3bm -e 0 --delete-zone 0 $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME $SRCDIR/data/emu3_test_add_zone_2'
test

cleanUp
