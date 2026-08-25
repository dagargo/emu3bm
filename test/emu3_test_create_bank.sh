#!/usr/bin/env bash

. $SRCDIR/test_common.sh

TEST_BANK_NAME=$(mktemp -d)/emu3_test_create_bank

logAndRun '$BINDIR/emu3bm -n $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME $SRCDIR/data/emu3_test_create_bank_esi2000'
test

logAndRun '$BINDIR/emu3bm -d esi2000 -n $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME $SRCDIR/data/emu3_test_create_bank_esi2000'
test

logAndRun '$BINDIR/emu3bm -d emu3x -n $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME $SRCDIR/data/emu3_test_create_bank_emu3x'
test

cleanUp
