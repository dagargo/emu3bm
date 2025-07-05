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

logAndRun '$srcdir/../../src/emu4bm -x ../data/emu4_test_add_sample_2'
test

logAndRun 'diff s1_loop.wav ../data/s1_loop.wav'
test

logAndRun 'diff s2_loop.wav ../data/s2_loop.wav'
test

logAndRun '$srcdir/../../src/emu4bm -X ../data/emu4_test_add_sample_2'
test

logAndRun 'diff 001-s1_loop.wav ../data/s1_loop.wav'
test

logAndRun 'diff 002-s2_loop.wav ../data/s2_loop.wav'
test

cleanUp
