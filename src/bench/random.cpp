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
#include <random>
#include <thread>
#include <unistd.h>

#include "dist_lsm/dist_lsm.h"
#include "k_lsm/k_lsm.h"
#include "pqs/cheap.h"
#include "pqs/globallock.h"
#include "pqs/linden.h"
#include "pqs/sequence_heap/sequence_heap.h"
#include "pqs/skip_list/skip_queue.h"
#include "pqs/spraylist.h"
#include "sequential_lsm/lsm.h"
#include "shared_lsm/shared_lsm.h"
#include "util.h"

constexpr int DEFAULT_SEED       = 0;
constexpr int DEFAULT_SIZE       = 1000000;  // Matches benchmarks from klsm paper.
constexpr int DEFAULT_NTHREADS   = 1;
constexpr int DEFAULT_RELAXATION = 256;
constexpr int DEFAULT_SLEEP      = 10;

#define PQ_CHEAP      "cheap"
#define PQ_DLSM       "dlsm"
#define PQ_GLOBALLOCK "globallock"
#define PQ_KLSM       "klsm"
#define PQ_LINDEN     "linden"
#define PQ_LSM        "lsm"
#define PQ_SEQUENCE   "sequence"
#define PQ_SKIP       "skip"
#define PQ_SLSM       "slsm"
#define PQ_SPRAY      "spray"

struct settings {
    int nthreads;
    int seed;
    int size;
    std::string type;
};

static hwloc_wrapper hwloc;

static std::atomic<int> fill_barrier;
static std::atomic<bool> start_barrier(false);
static std::atomic<bool> end_barrier(false);

class packed_uniform_bool_distribution {
public:
    packed_uniform_bool_distribution() :
        m_iteration(0)
    {
    }

    bool operator()(std::mt19937 &gen) {
        if (m_iteration == 0) {
            m_packed = m_rand_int(gen);
        }

        const bool ret = (m_packed >> m_iteration) & 1;
        m_iteration = (m_iteration + 1) & MASK;
        return ret;
    }

private:
    std::uniform_int_distribution<int64_t> m_rand_int;

    constexpr static int ITERATIONS = 64;
    constexpr static int MASK = ITERATIONS - 1;

    int m_iteration;
    int64_t m_packed;
};

static void
usage()
{
    fprintf(stderr,
            "USAGE: random [-s seed] [-p nthreads] [-i size] pq\n"
            "       -i: Specifies the initial size of the priority queue (default = %d)\n"
            "       -p: Specifies the number of threads (default = %d)\n"
            "       -s: Specifies the value used to seed the random number generator (default = %d)\n"
            "       pq: The data structure to use as the backing priority queue\n"
            "           (one of '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s')\n",
            DEFAULT_SIZE,
            DEFAULT_NTHREADS,
            DEFAULT_SEED,
            PQ_CHEAP, PQ_DLSM, PQ_GLOBALLOCK, PQ_KLSM, PQ_LINDEN,
            PQ_LSM, PQ_SEQUENCE, PQ_SKIP, PQ_SLSM);
    exit(EXIT_FAILURE);
}

template <class T>
static void
bench_thread(T *pq,
             const int thread_id,
             const struct settings &settings,
             std::promise<size_t> &&result)
{
    size_t nops = 0;

    std::mt19937 gen(settings.seed + thread_id);
    std::uniform_int_distribution<> rand_int;
    packed_uniform_bool_distribution rand_bool;

    hwloc.pin_to_core(thread_id);

    /* The spraylist requires per-thread initialization. */
    pq->init_thread(settings.nthreads);

    /* Fill up to initial size. Do this per thread in order to build a balanced DLSM
     * instead of having one local LSM containing all initial elems. */

    const int slice_size = settings.size / settings.nthreads;
    const int initial_size = (thread_id == settings.nthreads - 1) ?
                             settings.size - thread_id * slice_size : slice_size;
    const int initial_seed = settings.seed + thread_id + settings.nthreads;
    const auto initial_elems = random_array(initial_size, initial_seed);
    for (auto elem : initial_elems) {
        pq->insert(elem, elem);
    }
    fill_barrier.fetch_sub(1, std::memory_order_relaxed);

    while (!start_barrier.load(std::memory_order_relaxed)) {
        /* Wait. */
    }

    uint32_t v;
    while (!end_barrier.load(std::memory_order_relaxed)) {
        if (rand_bool(gen)) {
            v = rand_int(gen);
            pq->insert(v, v);
            nops++;
        } else {
            pq->delete_min(v);  // TODO: Metric for failed delete_mins.
            nops++;
        }
    }

    result.set_value(nops);
}

template <class T>
static int
bench(T *pq,
      const struct settings &settings)
{
    if (settings.nthreads > 1 && !pq->supports_concurrency()) {
        fprintf(stderr, "The given data structure does not support concurrency.\n");
        return -1;
    }

    int ret = 0;

    fill_barrier.store(settings.nthreads, std::memory_order_relaxed);

    /* Start all threads. */

    std::vector<std::future<size_t>> futures;
    std::vector<std::thread> threads(settings.nthreads);
    for (int i = 0; i < settings.nthreads; i++) {
        std::promise<size_t> p;
        futures.push_back(p.get_future());
        threads[i] = std::thread(bench_thread<T>, pq, i, settings, std::move(p));
    }

    /* Wait until threads are done filling their queue. */
    while (fill_barrier.load(std::memory_order_relaxed) > 0) {
        /* Wait. */
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
    size_t ops_per_s = (size_t)((double)nops / elapsed);

    fprintf(stdout, "%zu\n", ops_per_s);

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

    if (settings.type == PQ_CHEAP) {
        kpqbench::cheap<uint32_t, uint32_t> pq;
        ret = bench(&pq, settings);
    } else if (settings.type == PQ_DLSM) {
        kpq::dist_lsm<uint32_t, uint32_t, DEFAULT_RELAXATION> pq;
        ret = bench(&pq, settings);
    } else if (settings.type == PQ_GLOBALLOCK) {
        kpqbench::GlobalLock<uint32_t, uint32_t> pq;
        ret = bench(&pq, settings);
    } else if (settings.type == PQ_KLSM) {
        kpq::k_lsm<uint32_t, uint32_t, DEFAULT_RELAXATION> pq;
        ret = bench(&pq, settings);
    } else if (settings.type == PQ_LINDEN) {
        kpqbench::Linden pq(kpqbench::Linden::DEFAULT_OFFSET);
        pq.insert(42, 42); /* A hack to avoid segfault on destructor in empty linden queue. */
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
    } else if (settings.type == PQ_SLSM) {
        kpq::shared_lsm<uint32_t, uint32_t, DEFAULT_RELAXATION> pq;
        ret = bench(&pq, settings);
    } else if (settings.type == PQ_SPRAY) {
        kpqbench::spraylist pq;
        ret = bench(&pq, settings);
    } else {
        usage();
    }

    return ret;
}
