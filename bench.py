#!/usr/bin/env python2

import re
import subprocess

from optparse import OptionParser

ALGORITHMS = [ 'cheap'
             , 'dlsm'
             , 'globallock'
             , 'klsm'
             , 'linden'
             , 'lsm'
             , 'sequence'
             , 'skip'
             , 'slsm'
             , 'spray'
             ]

NELEMS = [ 50000, 66666, 100000, 131072, 500000, 524288
         , 1000000, 1048576, 4194304, 5000000, 8388608
         , 10000000
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

BIN = 'build/src/bench/random'

def bench(algorithm, nthreads, outfile):
    output = subprocess.check_output([ BIN
                                     , '-p', str(nthreads)
                                     , algorithm
                                     ])

    outstr = '%s, %d, %s' % (algorithm, nthreads, output.strip())

    print outstr
    f.write(outstr + '\n')

if __name__ == '__main__':
    parser = OptionParser()
    parser.add_option("-a", "--algorithms", dest = "algorithms", default = ",".join(ALGORITHMS),
            help = "Comma-separated list of %s" % ALGORITHMS)
    parser.add_option("-p", "--nthreads", dest = "nthreads", default = ",".join(map(str, NCPUS)),
            help = "Comma-separated list of thread counts")
    parser.add_option("-o", "--outfile", dest = "outfile", default = '/dev/null',
            help = "Write results to outfile")
    parser.add_option("-r", "--reps", dest = "reps", type = 'int', default = REPS,
            help = "Repetitions per run")
    (options, args) = parser.parse_args()

    algorithms = options.algorithms.split(',')
    for a in algorithms:
        if a not in ALGORITHMS:
            parser.error('Invalid algorithm')

    nthreads = list()
    for n in options.nthreads.split(','):
        try:
            nthreads.append(int(n))
        except:
            parser.error('Invalid element count')

    with open(options.outfile, 'a') as f:
        for a in algorithms:
            for n in nthreads:
                for r in xrange(options.reps):
                    bench(a, n, f)
