#!/usr/bin/env bash

function cleanUp() {
  rm -f $TEST_BANK_NAME
}

TEST_BANK_NAME=$srcdir/test_add_preset

cleanUp

$srcdir/../src/emu3bm -n $TEST_BANK_NAME
[ $? -ne 0 ] && cleanUp && exit -1

$srcdir/../src/emu3bm -p "Preset 0" $TEST_BANK_NAME
[ $? -ne 0 ] && cleanUp && exit -1
diff $TEST_BANK_NAME data/test_add_preset_1
[ $? -ne 0 ] && cleanUp && exit -1

$srcdir/../src/emu3bm -p "Preset 1" $TEST_BANK_NAME
[ $? -ne 0 ] && cleanUp && exit -1
diff $TEST_BANK_NAME data/test_add_preset_2
[ $? -ne 0 ] && cleanUp && exit -1

cleanUp
