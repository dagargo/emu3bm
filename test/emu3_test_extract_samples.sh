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

logAndRun '$srcdir/../../src/emu3bm -x ../data/emu3_test_add_sample_4'
test

logAndRun 'ls s1.wav'
test

logAndRun 'ls s2.wav'
test

logAndRun 'ls s1_loop.wav'
test

logAndRun 'ls s2_loop.wav'
test

logAndRun 'diff s1.wav ../data/s1.back.wav'
test

logAndRun 'diff s2.wav ../data/s2.back.wav'
test

logAndRun 'diff s1_loop.wav ../data/s1_loop.back.wav'
test

logAndRun 'diff s2_loop.wav ../data/s2_loop.back.wav'
test

logAndRun '$srcdir/../../src/emu3bm -X ../data/emu3_test_add_sample_4'
test

logAndRun 'ls 001-s1.wav'
test

logAndRun 'ls 002-s2.wav'
test

logAndRun 'ls 003-s1_loop.wav'
test

logAndRun 'ls 004-s2_loop.wav'
test

logAndRun 'diff 001-s1.wav ../data/s1.back.wav'
test

logAndRun 'diff 002-s2.wav ../data/s2.back.wav'
test

logAndRun 'diff 003-s1_loop.wav ../data/s1_loop.back.wav'
test

logAndRun 'diff 004-s2_loop.wav ../data/s2_loop.back.wav'
test

cleanUp
