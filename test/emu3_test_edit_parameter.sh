#!/usr/bin/env bash

. $SRCDIR/test_common.sh

TEST_BANK_NAME=$(mktemp -d)/emu3_test_edit_parameters

cp $SRCDIR/data/emu3_test_add_zone_3 $TEST_BANK_NAME
chmod u+w $TEST_BANK_NAME

logAndRun '$BINDIR/emu3bm -e 0 -q 25 -c 200 $TEST_BANK_NAME'
test
logAndRun 'diff $TEST_BANK_NAME $SRCDIR/data/emu3_test_edit_parameter_1'
test

cleanUp
