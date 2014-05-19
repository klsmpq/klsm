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
#include <future>
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

constexpr int DEFAULT_SEED     = 0;
constexpr int DEFAULT_SIZE     = 1 << 15;
constexpr int DEFAULT_NTHREADS = 1;
constexpr int DEFAULT_SLEEP    = 10;

#define PQ_CLSM       "clsm"
#define PQ_GLOBALLOCK "globallock"
#define PQ_LINDEN     "linden"
#define PQ_LSM        "lsm"
#define PQ_SEQUENCE   "sequence"
#define PQ_SKIP       "skip"

struct settings {
    int nthreads;
    int seed;
    int size;
    std::string type;
};

static std::atomic<bool> start_barrier(false);
static std::atomic<bool> end_barrier(false);

static void
usage()
{
    fprintf(stderr,
            "USAGE: random [-s seed] [-n nelems] [-p nthreads] [-i size] pq\n"
            "       -i: Specifies the initial size of the priority queue (default = %d)\n"
            "       -p: Specifies the number of threads (default = %d)\n"
            "       -s: Specifies the value used to seed the random number generator (default = %d)\n"
            "       pq: The data structure to use as the backing priority queue\n"
            "           (one of '%s', %s', '%s', '%s', '%s', '%s')\n",
            DEFAULT_SIZE,
            DEFAULT_NTHREADS,
            DEFAULT_SEED,
            PQ_CLSM, PQ_GLOBALLOCK, PQ_LINDEN, PQ_LSM, PQ_SEQUENCE, PQ_SKIP);
    exit(EXIT_FAILURE);
}

template <class T>
static void
bench_thread(T *pq,
             const int seed,
             std::promise<size_t> &&result)
{
    size_t nops = 0;

    std::mt19937 gen(seed);
    std::uniform_int_distribution<> rand_int;
    std::uniform_int_distribution<> rand_bool(0, 1);

    while (!start_barrier.load(std::memory_order_relaxed)) {
        /* Wait. */
    }

    while (!end_barrier.load(std::memory_order_relaxed)) {
        if (rand_bool(gen)) {
            pq->insert(rand_int(gen));
            nops++;
        } else {
            uint32_t v;
            if (pq->delete_min(v)) {
                nops++;
            }
        }
    }

    result.set_value(nops);
}

template <class T>
static int
bench(T *pq,
      const struct settings &settings)
{
    int ret = 0;

    /* Fill up to initial size. */

    const auto initial_elems = random_array(settings.size, settings.seed);
    for (auto elem : initial_elems) {
        pq->insert(elem);
    }

    /* Start all threads. */

    std::vector<std::future<size_t>> futures;
    std::vector<std::thread> threads;
    for (int i = 0; i < settings.nthreads; i++) {
        std::promise<size_t> p;
        futures.push_back(p.get_future());
        threads.push_back(std::thread(bench_thread<T>, pq, settings.seed + 1 + i, std::move(p)));
    }

    /* Begin benchmark. */
    start_barrier.store(true, std::memory_order_relaxed);

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    usleep(1000000 * DEFAULT_SLEEP);
    end_barrier.store(true, std::memory_order_relaxed);

    clock_gettime(CLOCK_MONOTONIC, &end);
    /* End benchmark. */

    for (auto &thread : threads) {
        thread.join();
    }

    size_t nops = 0;
    for (auto &future : futures) {
        nops += future.get();
    }

    const double elapsed = timediff_in_s(start, end);
    fprintf(stdout, "%1.8f, %zu\n", elapsed, nops);

    return ret;
}

int
main(int argc,
     char **argv)
{
    int ret = 0;
    struct settings settings = { DEFAULT_NTHREADS, DEFAULT_SEED, DEFAULT_SIZE, "" };

    int opt;
    while ((opt = getopt(argc, argv, "i:n:s:p:")) != -1) {
        switch (opt) {
        case 'i':
            errno = 0;
            settings.size = strtol(optarg, NULL, 0);
            if (errno != 0) {
                usage();
            }
            break;
        case 'p':
            errno = 0;
            settings.nthreads = strtol(optarg, NULL, 0);
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

    if (settings.nthreads < 1) {
        usage();
    }

    if (settings.size < 1) {
        usage();
    }

    if (optind != argc - 1) {
        usage();
    }

    settings.type = argv[optind];

    if (settings.type == PQ_CLSM) {
        kpq::clsm<uint32_t, uint32_t> pq;
        ret = bench(&pq, settings);
    } else if (settings.type == PQ_GLOBALLOCK) {
        kpq::GlobalLock pq;
        ret = bench(&pq, settings);
    } else if (settings.type == PQ_LINDEN) {
        kpq::Linden pq(kpq::Linden::DEFAULT_OFFSET);
        ret = bench(&pq, settings);
    } else if (settings.type == PQ_LSM) {
        kpq::LSM<uint32_t> pq;
        ret = bench(&pq, settings);
    } else if (settings.type == PQ_SEQUENCE) {
        kpq::sequence_heap<uint32_t> pq;
        ret = bench(&pq, settings);
    } else if (settings.type == PQ_SKIP) {
        kpq::skip_queue<uint32_t> pq;
        ret = bench(&pq, settings);
    } else {
        usage();
    }

    return ret;
}
