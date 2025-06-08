#!/usr/bin/env bash

function cleanUp() {
  echo "Cleaning up..."
  rm -f $TEST_BANK_NAME
}

function logAndRun() {
  echo "Running '$*'..."
  out=$(eval $*)
  err=$?
  [ -n "$out" ] && echo "$out"
  return $err
}

function testCommon() {
  [ $1 -eq 1 ] && ok=$((ok+1))
  total=$((total+1))
  if [ -t 1 ]; then
    if [ $1 -eq 1 ]; then
      printf "\033[0;32m"
    else
      printf "\033[0;31m"
    fi
  fi
  printf "Results: $ok/$total"
  if [ -t 1 ]; then
    printf "\033[0m"
  fi
  printf "\n\n"
  [ $1 -ne 1 ] && cleanUp && exit 1
}

function test() {
  [ $? -eq 0 ] && this=1 || this=0
  testCommon $this $1
}

function testError() {
  [ $? -ne 0 ] && this=1 || this=0
  testCommon $this $1
}

total=0
ok=0
