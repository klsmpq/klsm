#!/bin/bash

# Grabs cpu, pop & push throughput columns, and converts them to ops per second.
sed -e '/^test/d' -e '1d' -e 's/\t/ /g' $1 | awk '{ printf "pheet,%d,%d\n", $7, int(($8 + $9) / 10) }'
