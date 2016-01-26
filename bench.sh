#!/bin/bash

ALGORITHMS="klsm16 klsm128 klsm256 klsm4096 multiq globallock"
THREADS="1 2 3 5 10 15 20 25 30 35 40 45 50 55 60"
REPS="0 1 2 3 4 5 6 7 8 9"
BIN=build/src/bench/random

declare -A workloads
workloads=([0]="uni"
           [1]="spl"
          )

declare -A keygens
keygens=([0]="uni"
         [1]="asc"
         [2]="desc"
         [3]="rst8"
         [4]="rst16"
        )

function bench()
{
    for a in $ALGORITHMS; do
        for p in $THREADS; do
            echo "$a, $p threads, ${workloads[$1]} ${keygens[$2]}"
            for r in $REPS; do
                micnativeloadex $BIN -d 0 -a "-p $p -w $1 -k $2 $a" | head -1 | tee -a 20160126_pluto_${workloads[$1]}_${keygens[$2]}
            done
        done
    done
}

bench 0 0
bench 0 1
bench 1 0
bench 1 1
bench 0 3
bench 0 4
