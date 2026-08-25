#!/usr/bin/env bash

. $SRCDIR/test_common.sh
ABS_SRCDIR=$(pwd)/$SRCDIR

function cleanUp() {
  rm -rf $EXT_DIR
}

EXT_DIR=$(mktemp -d)

cd $EXT_DIR

logAndRun '$BINDIR/emu4bm -x $ABS_SRCDIR/data/emu4_test_add_sample_2'
test

logAndRun 'diff s1_loop.wav $ABS_SRCDIR/data/s1_loop.back.wav'
test

logAndRun 'diff s2_loop.wav $ABS_SRCDIR/data/s2_loop.back.wav'
test

logAndRun '$BINDIR/emu4bm -X $ABS_SRCDIR/data/emu4_test_add_sample_2'
test

logAndRun 'diff 001-s1_loop.wav $ABS_SRCDIR/data/s1_loop.back.wav'
test

logAndRun 'diff 002-s2_loop.wav $ABS_SRCDIR/data/s2_loop.back.wav'
test

cleanUp
