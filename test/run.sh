#!/bin/bash

for i in `seq 1 31`; do
  echo "Running $i..."
  ../build/src/Driver/gross test$(printf "%03d" $i).txt
  echo "-----------"
done
