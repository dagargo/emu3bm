#!/usr/bin/env bash

function cleanUp() {
  rm -f $TEST_BANK_NAME
}

TEST_BANK_NAME=$srcdir/test_create_bank

cleanUp

$srcdir/../src/emu3bm -n $TEST_BANK_NAME
[ $? -ne 0 ] && cleanUp && exit -1
diff $TEST_BANK_NAME $srcdir/data/test_create_bank_esi2000
v=$?
cleanUp

$srcdir/../src/emu3bm -d esi2000 -n $TEST_BANK_NAME
[ $? -ne 0 ] && cleanUp && exit -1
diff $TEST_BANK_NAME $srcdir/data/test_create_bank_esi2000
v=$?
cleanUp

$srcdir/../src/emu3bm -d emu3x -n $TEST_BANK_NAME
[ $? -ne 0 ] && cleanUp && exit -1
diff $TEST_BANK_NAME $srcdir/data/test_create_bank_emu3x
v=$?
cleanUp

exit $v
