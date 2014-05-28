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
#include <future>
#include <getopt.h>
#include <hwloc.h>
#include <random>
#include <thread>
#include <unistd.h>

#include "clsm/clsm.h"
#include "globallock.h"
#include "linden.h"
#include "lsm.h"
#include "sequence_heap/sequence_heap.h"
#include "skip_list/skip_queue.h"
#include "util.h"

constexpr int DEFAULT_NNODES    = 1 << 15;
constexpr int DEFAULT_NTHREADS  = 1;
constexpr double DEFAULT_EDGE_P = 0.5;
constexpr int DEFAULT_SEED      = 0;

#define PQ_CLSM       "clsm"
#define PQ_GLOBALLOCK "globallock"
#define PQ_LINDEN     "linden"
#define PQ_LSM        "lsm"
#define PQ_SEQUENCE   "sequence"
#define PQ_SKIP       "skip"

struct settings {
    int num_nodes;
    int num_threads;
    double edge_probability;
    int seed;
    std::string type;
};

static hwloc_topology_t topology;

static void
usage()
{
    fprintf(stderr,
            "USAGE: shortest_paths [-m num_nodes] [-n num_threads] [-p edge_probability]\n"
            "                      [-s seed] pq\n"
            "       -m: The number of nodes generated (default = %d)\n"
            "       -n: Number of threads (default = %d)\n"
            "       -p: The probability of an edge between two nodes (default = %f)\n"
            "       -s: The random number generator seed (default = %d)\n"
            "       pq: The data structure to use as the backing priority queue\n"
            "           (one of '%s', %s', '%s', '%s', '%s', '%s')\n",
            DEFAULT_NNODES,
            DEFAULT_NTHREADS,
            DEFAULT_EDGE_P,
            DEFAULT_SEED,
            PQ_CLSM, PQ_GLOBALLOCK, PQ_LINDEN, PQ_LSM, PQ_SEQUENCE, PQ_SKIP);
    exit(EXIT_FAILURE);
}

template <class T>
static int
bench(T *pq,
      const struct settings &settings)
{
    int ret = 0;

    return ret;
}

int
main(int argc,
     char **argv)
{
    int ret = 0;
    struct settings s = { DEFAULT_NNODES, DEFAULT_NTHREADS, DEFAULT_EDGE_P, DEFAULT_SEED, ""};

    int opt;
    while ((opt = getopt(argc, argv, "m:n:p:s:")) != -1) {
        switch (opt) {
        case 'm':
            errno = 0;
            s.num_nodes = strtol(optarg, NULL, 0);
            if (errno != 0) {
                usage();
            }
            break;
        case 'n':
            errno = 0;
            s.num_threads = strtol(optarg, NULL, 0);
            if (errno != 0) {
                usage();
            }
            break;
        case 'p':
            errno = 0;
            s.edge_probability = strtod(optarg, NULL);
            if (errno != 0) {
                usage();
            }
            break;
        case 's':
            errno = 0;
            s.seed = strtol(optarg, NULL, 0);
            if (errno != 0) {
                usage();
            }
            break;
        default:
            usage();
        }
    }

    if (s.num_nodes < 1) {
        usage();
    }

    if (s.num_threads < 1) {
        usage();
    }

    if (s.edge_probability <= 0.0 || s.edge_probability > 1.0) {
        usage();
    }

    if (optind != argc - 1) {
        usage();
    }

    s.type = argv[optind];

    hwloc_topology_init(&topology);
    hwloc_topology_load(topology);

    if (s.type == PQ_CLSM) {
        kpq::clsm<uint32_t, uint32_t> pq;
        ret = bench(&pq, s);
    } else if (s.type == PQ_GLOBALLOCK) {
        kpq::GlobalLock pq;
        ret = bench(&pq, s);
    } else if (s.type == PQ_LINDEN) {
        kpq::Linden pq(kpq::Linden::DEFAULT_OFFSET);
        pq.insert(42); /* A hack to avoid segfault on destructor in empty linden queue. */
        ret = bench(&pq, s);
    } else if (s.type == PQ_LSM) {
        kpq::LSM<uint32_t> pq;
        ret = bench(&pq, s);
    } else if (s.type == PQ_SEQUENCE) {
        kpq::sequence_heap<uint32_t> pq;
        ret = bench(&pq, s);
    } else if (s.type == PQ_SKIP) {
        kpq::skip_queue<uint32_t> pq;
        ret = bench(&pq, s);
    } else {
        usage();
    }

    hwloc_topology_destroy(topology);

    return ret;
}
