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

logAndRun '$srcdir/../../src/emu3bm -x ../data/test_add_sample_4'
test

logAndRun 'diff s1.wav ../data/s1.wav'
test

logAndRun 'diff s1.wav ../data/s1.wav'
test

logAndRun 'diff s2.wav ../data/s2.wav'
test

logAndRun 'diff s2.wav ../data/s2.wav'
test

logAndRun '$srcdir/../../src/emu3bm -X ../data/test_add_sample_4'
test

logAndRun 'diff 001-s1.wav ../data/s1.wav'
test

logAndRun 'diff 002-s1.wav ../data/s1.wav'
test

logAndRun 'diff 003-s2.wav ../data/s2.wav'
test

logAndRun 'diff 004-s2.wav ../data/s2.wav'
test

cleanUp
