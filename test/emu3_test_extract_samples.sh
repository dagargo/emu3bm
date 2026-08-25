#!/usr/bin/env bash

. $SRCDIR/test_common.sh
ABS_SRCDIR=$(pwd)/$SRCDIR

function cleanUp() {
  rm -rf $EXT_DIR
}

EXT_DIR=$(mktemp -d)

cd $EXT_DIR

logAndRun '$BINDIR/emu3bm -x $ABS_SRCDIR/data/emu3_test_add_sample_4'
test

logAndRun 'ls s1.wav'
test

logAndRun 'ls s2.wav'
test

logAndRun 'ls s1_loop.wav'
test

logAndRun 'ls s2_loop.wav'
test

logAndRun 'diff s1.wav $ABS_SRCDIR/data/s1.back.wav'
test

logAndRun 'diff s2.wav $ABS_SRCDIR/data/s2.back.wav'
test

logAndRun 'diff s1_loop.wav $ABS_SRCDIR/data/s1_loop.back.wav'
test

logAndRun 'diff s2_loop.wav $ABS_SRCDIR/data/s2_loop.back.wav'
test

logAndRun '$BINDIR/emu3bm -X $ABS_SRCDIR/data/emu3_test_add_sample_4'
test

logAndRun 'ls 001-s1.wav'
test

logAndRun 'ls 002-s2.wav'
test

logAndRun 'ls 003-s1_loop.wav'
test

logAndRun 'ls 004-s2_loop.wav'
test

logAndRun 'diff 001-s1.wav $ABS_SRCDIR/data/s1.back.wav'
test

logAndRun 'diff 002-s2.wav $ABS_SRCDIR/data/s2.back.wav'
test

logAndRun 'diff 003-s1_loop.wav $ABS_SRCDIR/data/s1_loop.back.wav'
test

logAndRun 'diff 004-s2_loop.wav $ABS_SRCDIR/data/s2_loop.back.wav'
test

cd -

cleanUp
