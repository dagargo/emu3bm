#!/usr/bin/env bash

. $srcdir/test_common.sh

TEST_BANK_NAME=$srcdir/emu3_test_add_sfz

cleanUp

logAndRun '$srcdir/../src/emu3bm -n $TEST_BANK_NAME'
test

logAndRun '$srcdir/../src/emu3bm -S data/test_grammar.sfz $TEST_BANK_NAME'
test

cleanUp
