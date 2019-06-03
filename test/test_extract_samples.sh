#!/usr/bin/env bash

function cleanUp() {
  cd -
  rm -rf $EXT_DIR
}

EXT_DIR=extdir

rm -rf $EXT_DIR

mkdir $EXT_DIR
cd $EXT_DIR

$srcdir/../../src/emu3bm -x ../data/test_add_sample_4
[ $? -ne 0 ] && cleanUp && exit -1

diff s1.wav ../data/s1.wav
[ $? -ne 0 ] && cleanUp && exit -1

diff s1.wav ../data/s1.wav
[ $? -ne 0 ] && cleanUp && exit -1

diff s2.wav ../data/s2.wav
[ $? -ne 0 ] && cleanUp && exit -1

diff s2.wav ../data/s2.wav
[ $? -ne 0 ] && cleanUp && exit -1

$srcdir/../../src/emu3bm -X ../data/test_add_sample_4
[ $? -ne 0 ] && cleanUp && exit -1

diff 001-s1.wav ../data/s1.wav
[ $? -ne 0 ] && cleanUp && exit -1

diff 002-s1.wav ../data/s1.wav
[ $? -ne 0 ] && cleanUp && exit -1

diff 003-s2.wav ../data/s2.wav
[ $? -ne 0 ] && cleanUp && exit -1

diff 004-s2.wav ../data/s2.wav
[ $? -ne 0 ] && cleanUp && exit -1

cleanUp
