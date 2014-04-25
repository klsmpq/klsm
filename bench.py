#!/usr/bin/env python2

import re
import subprocess

from optparse import OptionParser

ALGORITHMS = [ 'globallock'
             , 'lsm'
             ]

NELEMS = [ 100, 512, 1000, 1024
         , 4096, 10000, 50000, 100000
         , 1000000, 10000000
         ]

NCPUS = [  1,  2,  3
        ,  4,  6,  8, 10
        , 11, 15, 20
        , 21, 25, 30
        , 31, 35, 40
        , 41, 45, 50
        , 51, 55, 60
        , 61, 65, 70
        , 71, 75, 80
        ]

REPS = 5

BIN = 'build/src/bench/heapsort'

def bench(algorithm, nelems, outfile):
    output = subprocess.check_output([ BIN
                                     , '-n', str(nelems)
                                     , algorithm
                                     ])

    outstr = '%s, %d, %s' % (algorithm, nelems, output.strip())

    print outstr
    f.write(outstr + '\n')

if __name__ == '__main__':
    parser = OptionParser()
    parser.add_option("-a", "--algorithms", dest = "algorithms", default = ",".join(ALGORITHMS),
            help = "Comma-separated list of %s" % ALGORITHMS)
    parser.add_option("-n", "--nelems", dest = "nelems", default = ",".join(map(str, NELEMS)),
            help = "Comma-separated list of element counts")
    parser.add_option("-o", "--outfile", dest = "outfile", default = '/dev/null',
            help = "Write results to outfile")
    parser.add_option("-r", "--reps", dest = "reps", type = 'int', default = REPS,
            help = "Repetitions per run")
    (options, args) = parser.parse_args()

    algorithms = options.algorithms.split(',')
    for a in algorithms:
        if a not in ALGORITHMS:
            parser.error('Invalid algorithm')

    nelems = list()
    for n in options.nelems.split(','):
        try:
            nelems.append(int(n))
        except:
            parser.error('Invalid element count')

    with open(options.outfile, 'a') as f:
        for a in algorithms:
            for n in nelems:
                for r in xrange(options.reps):
                    bench(a, n, f)
