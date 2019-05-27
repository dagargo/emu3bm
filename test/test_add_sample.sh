#!/usr/bin/env bash

function cleanUp() {
  rm -f $TEST_BANK_NAME
}

TEST_BANK_NAME=$srcdir/test_add_sample

cleanUp

$srcdir/../src/emu3bm -n $TEST_BANK_NAME
[ $? -ne 0 ] && cleanUp && exit -1

$srcdir/../src/emu3bm -s foo $TEST_BANK_NAME
[ $? -eq 0 ] && cleanUp && exit -1

$srcdir/../src/emu3bm -s data/s1.wav $TEST_BANK_NAME
[ $? -ne 0 ] && cleanUp && exit -1
diff $TEST_BANK_NAME data/test_add_sample_1
[ $? -ne 0 ] && cleanUp && exit -1

$srcdir/../src/emu3bm -S data/s1.wav $TEST_BANK_NAME
[ $? -ne 0 ] && cleanUp && exit -1
diff $TEST_BANK_NAME data/test_add_sample_2
[ $? -ne 0 ] && cleanUp && exit -1

$srcdir/../src/emu3bm -s data/s2.wav $TEST_BANK_NAME
[ $? -ne 0 ] && cleanUp && exit -1
diff $TEST_BANK_NAME data/test_add_sample_3
[ $? -ne 0 ] && cleanUp && exit -1

$srcdir/../src/emu3bm -S data/s2.wav $TEST_BANK_NAME
[ $? -ne 0 ] && cleanUp && exit -1
diff $TEST_BANK_NAME data/test_add_sample_4
[ $? -ne 0 ] && cleanUp && exit -1

cleanUp
