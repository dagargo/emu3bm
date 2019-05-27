#!/usr/bin/env bash

function cleanUp() {
  rm -f $TEST_BANK_NAME
}

TEST_BANK_NAME=$srcdir/test_create_bank

cleanUp

$srcdir/../src/emu3bm -n $TEST_BANK_NAME
[ $? -ne 0 ] && cleanUp && exit -1
diff $TEST_BANK_NAME $srcdir/data/test_create_bank
v=$?
cleanUp

exit $v
