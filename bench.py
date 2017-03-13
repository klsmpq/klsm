#!/usr/bin/env python2

import re
import subprocess

from optparse import OptionParser

ALGORITHMS = [ 'cadm'
             , 'cain'
             , 'capq'
             , 'catree'
             , 'cheap'
             , 'dlsm'
             , 'globallock'
             , 'klsm16'
             , 'klsm128'
             , 'klsm256'
             , 'klsm4096'
             , 'linden'
             , 'lsm'
             , 'mlsm'
             , 'multiq'
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

KEYGEN = 0
WORKLOAD = 0

BIN = 'build/src/bench/random'

def bench(algorithm, nthreads, seed, outfile, options):
    output = subprocess.check_output([ BIN
                                     , '-k', str(options.keygen)
                                     , '-p', str(nthreads)
                                     , '-s', str(seed)
                                     , '-w', str(options.workload)
                                     , algorithm
                                     ])

    outstr = '%s, %d, %s' % (algorithm, nthreads, output.strip())

    print outstr
    f.write(outstr + '\n')

if __name__ == '__main__':
    parser = OptionParser()
    parser.add_option("-a", "--algorithms", dest = "algorithms", default = ",".join(ALGORITHMS),
            help = "Comma-separated list of %s" % ALGORITHMS)
    parser.add_option("-k", "--keygen", dest = "keygen", default = KEYGEN,
            help = "Keygen (0: uniform, 1: ascending, 2: descending, 3: restricted8, 4:restricted16)")
    parser.add_option("-p", "--nthreads", dest = "nthreads", default = ",".join(map(str, NCPUS)),
            help = "Comma-separated list of thread counts")
    parser.add_option("-o", "--outfile", dest = "outfile", default = '/dev/null',
            help = "Write results to outfile")
    parser.add_option("-r", "--reps", dest = "reps", type = 'int', default = REPS,
            help = "Repetitions per run")
    parser.add_option("-w", "--workload", dest = "workload", type = 'int', default = WORKLOAD,
            help = "Workload (0: uniform, 1: split, 2: producer)")
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
                    bench(a, n, r, f, options)
