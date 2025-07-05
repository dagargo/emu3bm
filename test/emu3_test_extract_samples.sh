#!/usr/bin/env bash

. $srcdir/test_common.sh

function cleanUp() {
  cd -
  rm -rf $EXT_DIR
}

EXT_DIR=extdir

rm -rf $EXT_DIR

mkdir $EXT_DIR
cd $EXT_DIR

logAndRun '$srcdir/../../src/emu3bm -x ../data/emu3_test_add_sample_6'
test

logAndRun 'ls s1.wav'
test

logAndRun 'ls s2.wav'
test

#Other samples sharing the same names names are being overwritten.

logAndRun 'diff s1_loop.wav ../data/s1_loop.wav'
test

logAndRun 'diff s2_loop.wav ../data/s2_loop.wav'
test

logAndRun '$srcdir/../../src/emu3bm -X ../data/emu3_test_add_sample_6'
test

logAndRun 'ls 001-s1.wav'
test

logAndRun 'ls 002-s1.wav'
test

logAndRun 'ls 003-s2.wav'
test

logAndRun 'ls 004-s2.wav'
test

logAndRun 'diff 005-s1_loop.wav ../data/s1_loop.wav'
test

logAndRun 'diff 006-s2_loop.wav ../data/s2_loop.wav'
test

cleanUp
