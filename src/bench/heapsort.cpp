/*
 *  Copyright 2014 Jakob Gruber
 *
 *  This file is part of kpqueue.
 *
 *  kpqueue is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  kpqueue is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with kpqueue.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <ctime>
#include <getopt.h>
#include <random>

#include "pqs/globallock.h"
#include "pqs/sequence_heap.h"
#include "pqs/skip_queue.h"
#include "sequential_lsm/lsm.h"
#include "util.h"

constexpr int DEFAULT_SEED = 0;
constexpr int DEFAULT_NELEMS = 1 << 15;

#define PQ_GLOBALLOCK "globallock"
#define PQ_LSM        "lsm"
#define PQ_SEQUENCE   "sequence"
#define PQ_SKIP       "skip"

struct settings {
    std::string type;
    int seed;
    int nelems;
};

static void
usage()
{
    fprintf(stderr,
            "USAGE: heapsort [-s seed] [-n nelems] pq\n"
            "       -s: Specifies the value used to seed the random number generator (default = %d)\n"
            "       -n: Specifies the number of elements to sort (default = %d)\n"
            "       pq: The data structure to use as the backing priority queue\n"
            "           (one of '%s', '%s', '%s', '%s')\n",
            DEFAULT_SEED,
            DEFAULT_NELEMS,
            PQ_GLOBALLOCK, PQ_LSM, PQ_SEQUENCE, PQ_SKIP);
    exit(EXIT_FAILURE);
}

template <class T>
static int
bench(T *pq,
      const struct settings &settings)
{
    int ret = 0;

    std::vector<uint32_t> xs = random_array(settings.nelems, settings.seed);

    /* Begin benchmark. */
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < settings.nelems; i++) {
        pq->insert(xs[i], xs[i]);
    }

    for (int i = 0; i < settings.nelems; i++) {
        pq->delete_min(xs[i]);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    /* End benchmark. */

    const double elapsed = timediff_in_s(start, end);
    fprintf(stdout, "%1.8f\n", elapsed);

    /* Verify results. */
    for (int i = 1; i < settings.nelems; i++) {
        if (xs[i] < xs[i - 1]) {
            fprintf(stderr, "INVALID RESULTS: xs[%d] (%d) < xs[%d] (%d)\n", i, xs[i], i - 1, xs[i - 1]);
            ret = -1;
        }
    }

    return ret;
}

int
main(int argc,
     char **argv)
{
    int ret = 0;
    struct settings settings = { "", DEFAULT_SEED, DEFAULT_NELEMS };

    int opt;
    while ((opt = getopt(argc, argv, "n:s:")) != -1) {
        switch (opt) {
        case 'n':
            errno = 0;
            settings.nelems = strtol(optarg, NULL, 0);
            if (errno != 0) {
                usage();
            }
            break;
        case 's':
            errno = 0;
            settings.seed = strtol(optarg, NULL, 0);
            if (errno != 0) {
                usage();
            }
            break;
        default:
            usage();
        }
    }

    if (settings.nelems < 1) {
        usage();
    }

    if (optind != argc - 1) {
        usage();
    }

    settings.type = argv[optind];
    if (settings.type == PQ_GLOBALLOCK) {
        kpqbench::GlobalLock<uint32_t, uint32_t> pq;
        ret = bench(&pq, settings);
    } else if (settings.type == PQ_LSM) {
        kpq::LSM<uint32_t> pq;
        ret = bench(&pq, settings);
    } else if (settings.type == PQ_SEQUENCE) {
        kpqbench::sequence_heap<uint32_t> pq;
        ret = bench(&pq, settings);
    } else if (settings.type == PQ_SKIP) {
        kpqbench::skip_queue<uint32_t> pq;
        ret = bench(&pq, settings);
    } else {
        usage();
    }

    return ret;
}
