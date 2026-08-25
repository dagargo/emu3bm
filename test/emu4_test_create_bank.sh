#!/usr/bin/env bash

. $SRCDIR/test_common.sh

TEST_BANK_NAME=$(mktemp -d)/emu4_test_create_bank

logAndRun '$BINDIR/emu4bm -n $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME $SRCDIR/data/emu4_test_create_bank'
test

cleanUp
