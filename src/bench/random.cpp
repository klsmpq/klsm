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

#ifdef HAVE_VALGRIND
#include <valgrind/callgrind.h>
#endif

#include "dist_lsm/dist_lsm.h"
#include "k_lsm/k_lsm.h"
#include "pqs/cheap.h"
#include "pqs/globallock.h"
#include "pqs/linden.h"
#include "pqs/multiq.h"
#include "pqs/sequence_heap.h"
#include "pqs/skip_queue.h"
#include "pqs/spraylist.h"
#include "sequential_lsm/lsm.h"
#include "shared_lsm/shared_lsm.h"
#include "util/counters.h"
#include "util.h"

#define PQ_CHEAP      "cheap"
#define PQ_DLSM       "dlsm"
#define PQ_GLOBALLOCK "globallock"
#define PQ_KLSM16     "klsm16"
#define PQ_KLSM128    "klsm128"
#define PQ_KLSM256    "klsm256"
#define PQ_KLSM4096   "klsm4096"
#define PQ_LINDEN     "linden"
#define PQ_LSM        "lsm"
#define PQ_MULTIQ     "multiq"
#define PQ_SEQUENCE   "sequence"
#define PQ_SKIP       "skip"
#define PQ_SLSM       "slsm"
#define PQ_SPRAY      "spray"

/**
 * Uniform: Each thread performs 50% inserts, 50% deletes.
 * Split: 50% of threads perform inserts, 50% of threads perform deletes (in case of an
 *        odd thread count there are more inserts than deletes).
 * Producer: A single thread performs inserts, all others delete.
 */
enum {
    WORKLOAD_UNIFORM = 0,
    WORKLOAD_SPLIT,
    WORKLOAD_PRODUCER,
    WORKLOAD_COUNT,
};

/**
 * Uniform: Keys are generated uniformly at random.
 * Ascending: Keys are generated uniformly at random within a smaller integer range [x, x + z]
 *            s.t. x rises over time.
 */
enum {
    KEYS_UNIFORM = 0,
    KEYS_ASCENDING,
    KEYS_COUNT,
};

constexpr int DEFAULT_SEED       = 0;
constexpr int DEFAULT_SIZE       = 1000000;  // Matches benchmarks from klsm paper.
constexpr int DEFAULT_NTHREADS   = 1;
constexpr int DEFAULT_RELAXATION = 256;
constexpr int DEFAULT_SLEEP      = 10;
constexpr auto DEFAULT_COUNTERS  = false;
constexpr auto DEFAULT_WORKLOAD  = WORKLOAD_UNIFORM;
constexpr auto DEFAULT_KEYS      = KEYS_UNIFORM;

struct settings {
    int nthreads;
    int seed;
    int size;
    std::string type;
    bool print_counters;
    int keys;
    int workload;

    bool are_valid() const {
        if (nthreads < 1
                || size < 1
                || keys < 0 || keys >= KEYS_COUNT
                || workload < 0 || workload >= WORKLOAD_COUNT) {
            return false;
        }

        return true;
    }
};

static hwloc_wrapper hwloc;

static std::atomic<int> fill_barrier;
static std::atomic<bool> start_barrier(false);
static std::atomic<bool> end_barrier(false);

class packed_uniform_bool_distribution {
public:
    packed_uniform_bool_distribution() :
        m_rand_int(0ULL, std::numeric_limits<uint64_t>::max()),
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
    std::uniform_int_distribution<uint64_t> m_rand_int;

    constexpr static int ITERATIONS = 64;
    constexpr static int MASK = ITERATIONS - 1;

    int m_iteration;
    int64_t m_packed;
};

static void
usage()
{
    fprintf(stderr,
            "USAGE: random [-c] [-i size] [-k keys] [-p nthreads] [-s seed] [-w workload] pq\n"
            "       -c: Print performance counters (default = %d)\n"
            "       -i: Specifies the initial size of the priority queue (default = %d)\n"
            "       -k: Specifies the key generation type, one of %d: uniform, %d: ascending (default = %d)\n"
            "       -p: Specifies the number of threads (default = %d)\n"
            "       -s: Specifies the value used to seed the random number generator (default = %d)\n"
            "       -w: Specifies the workload type, one of %d: uniform, %d: split, %d: producer (default = %d)\n"
            "       pq: The data structure to use as the backing priority queue\n"
            "           (one of '%s', '%s', '%s', '%s', '%s', '%s',\n"
            "                   '%s', '%s', '%s', '%s', '%s', '%s',\n"
            "                   '%s', '%s')\n",
            DEFAULT_COUNTERS,
            DEFAULT_SIZE,
            KEYS_UNIFORM, KEYS_ASCENDING, DEFAULT_KEYS,
            DEFAULT_NTHREADS,
            DEFAULT_SEED,
            WORKLOAD_UNIFORM, WORKLOAD_SPLIT, WORKLOAD_PRODUCER, DEFAULT_WORKLOAD,
            PQ_CHEAP, PQ_DLSM, PQ_GLOBALLOCK, PQ_KLSM16, PQ_KLSM128, PQ_KLSM256, PQ_KLSM4096,
            PQ_LINDEN, PQ_LSM, PQ_MULTIQ, PQ_SEQUENCE, PQ_SKIP, PQ_SLSM, PQ_SPRAY);
    exit(EXIT_FAILURE);
}

class workload_uniform {
public:
    workload_uniform(const struct settings &settings,
                     const int thread_id) :
        m_gen(settings.seed + thread_id)
    {
    }

    bool insert() { return m_rand_bool(m_gen); }

private:
    std::mt19937 m_gen;
    packed_uniform_bool_distribution m_rand_bool;
};

class workload_split {
public:
    workload_split(const struct settings &,
                   const int thread_id) :
        m_thread_id(thread_id)
    {
    }

    bool insert() const { return ((m_thread_id & 1) == 0); }

private:
    const int m_thread_id;
};

class workload_producer {
public:
    workload_producer(const struct settings &,
                      const int thread_id) :
        m_thread_id(thread_id)
    {
    }

    bool insert() const { return (m_thread_id == 0); }

private:
    const int m_thread_id;
};

class keygen_uniform {
public:
    keygen_uniform(const struct settings &settings,
                          const int thread_id) :
        m_gen(settings.seed + thread_id)
    {
    }

    uint32_t next()
    {
        return m_rand_int(m_gen);
    }

private:
    std::mt19937 m_gen;
    std::uniform_int_distribution<> m_rand_int;
};

class keygen_ascending {
public:
    keygen_ascending(const struct settings &settings,
                     const int thread_id) :
        m_gen(settings.seed + thread_id),
        m_rand_int(0, UPPER_BOUND),
        m_base(0)
    {
    }

    uint32_t next()
    {
        return m_rand_int(m_gen) + m_base++;
    }

private:
    static constexpr uint32_t UPPER_BOUND = 512;

    std::mt19937 m_gen;
    std::uniform_int_distribution<uint32_t> m_rand_int;
    uint32_t m_base;
};

template <class PriorityQueue, class WorkLoad, class KeyGeneration>
static void
bench_thread(PriorityQueue *pq,
             const int thread_id,
             const struct settings &settings,
             std::promise<kpq::counters> &&result)
{
#ifdef HAVE_VALGRIND
    CALLGRIND_STOP_INSTRUMENTATION;
#endif
    WorkLoad workload(settings, thread_id);
    KeyGeneration keygen(settings, thread_id);

    hwloc.pin_to_core(thread_id);

    /* The spraylist requires per-thread initialization. */
    pq->init_thread(settings.nthreads);

    /* Fill up to initial size. Do this per thread in order to build a balanced DLSM
     * instead of having one local LSM containing all initial elems. */

    const int slice_size = settings.size / settings.nthreads;
    const int initial_size = (thread_id == settings.nthreads - 1) ?
                             settings.size - thread_id * slice_size : slice_size;
    for (int i = 0; i < initial_size; i++) {
        uint32_t elem = keygen.next();
        pq->insert(elem, elem);
    }
    fill_barrier.fetch_sub(1, std::memory_order_relaxed);

#ifdef HAVE_VALGRIND
    CALLGRIND_START_INSTRUMENTATION;
#endif

    while (!start_barrier.load(std::memory_order_relaxed)) {
        /* Wait. */
    }

#ifdef HAVE_VALGRIND
    CALLGRIND_ZERO_STATS;
#endif

    uint32_t v;
    while (!end_barrier.load(std::memory_order_relaxed)) {
        if (workload.insert()) {
            v = keygen.next();
            pq->insert(v, v);
            kpq::COUNTERS.inserts++;
        } else {
            /* TODO: Possibly revert to including failed deletes in the operation count
             * (pheet does this). However, it is important to distinguish failed
             * deletes at this time since spy() is currently disabled. */
            if (pq->delete_min(v)) {
                kpq::COUNTERS.successful_deletes++;
            } else {
                kpq::COUNTERS.failed_deletes++;
            }
        }
    }

#ifdef HAVE_VALGRIND
    CALLGRIND_STOP_INSTRUMENTATION;
#endif

    result.set_value(kpq::COUNTERS);
}

template <class PriorityQueue>
static int
bench(PriorityQueue *pq,
      const struct settings &settings)
{
    if (settings.nthreads > 1 && !pq->supports_concurrency()) {
        fprintf(stderr, "The given data structure does not support concurrency.\n");
        return -1;
    }

    int ret = 0;

    fill_barrier.store(settings.nthreads, std::memory_order_relaxed);

    /* Start all threads. */

    auto fn = bench_thread<PriorityQueue, workload_uniform, keygen_uniform>;
    switch (settings.workload) {
    case WORKLOAD_UNIFORM:
        switch (settings.keys) {
        case KEYS_UNIFORM:
            fn = bench_thread<PriorityQueue, workload_uniform, keygen_uniform>; break;
        case KEYS_ASCENDING:
            fn = bench_thread<PriorityQueue, workload_uniform, keygen_ascending>; break;
        default: assert(false);
        }
        break;
    case WORKLOAD_SPLIT:
        switch (settings.keys) {
        case KEYS_UNIFORM:
            fn = bench_thread<PriorityQueue, workload_split, keygen_uniform>; break;
        case KEYS_ASCENDING:
            fn = bench_thread<PriorityQueue, workload_split, keygen_ascending>; break;
        default: assert(false);
        }
        break;
    case WORKLOAD_PRODUCER:
        switch (settings.keys) {
        case KEYS_UNIFORM:
            fn = bench_thread<PriorityQueue, workload_producer, keygen_uniform>; break;
        case KEYS_ASCENDING:
            fn = bench_thread<PriorityQueue, workload_producer, keygen_ascending>; break;
        default: assert(false);
        }
        break;
    default: assert(false);
    }

    std::vector<std::future<kpq::counters>> futures;
    std::vector<std::thread> threads(settings.nthreads);
    for (int i = 0; i < settings.nthreads; i++) {
        std::promise<kpq::counters> p;
        futures.push_back(p.get_future());
        threads[i] = std::thread(fn, pq, i, settings, std::move(p));
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

    kpq::counters counters;
    for (auto &future : futures) {
        counters += future.get();
    }

    const double elapsed = timediff_in_s(start, end);
    size_t ops_per_s = (size_t)((double)counters.operations() / elapsed);

    fprintf(stdout, "%zu\n", ops_per_s);

    if (settings.print_counters) {
        counters.print();
    }

    return ret;
}

static int safe_parse_int_arg(const char *arg)
{
    errno = 0;
    const int i = strtol(arg, NULL, 0);
    if (errno != 0) {
        usage();
    }
    return i;
}

int
main(int argc,
     char **argv)
{
    int ret = 0;
    struct settings settings = { DEFAULT_NTHREADS
                               , DEFAULT_SEED
                               , DEFAULT_SIZE
                               , ""
                               , DEFAULT_COUNTERS
                               , DEFAULT_KEYS
                               , DEFAULT_WORKLOAD
                               };

    int opt;
    while ((opt = getopt(argc, argv, "ci:k:n:p:s:w:")) != -1) {
        switch (opt) {
        case 'c':
            settings.print_counters = true;
            break;
        case 'i':
            settings.size = safe_parse_int_arg(optarg);
            break;
        case 'k':
            settings.keys = safe_parse_int_arg(optarg);
            break;
        case 'p':
            settings.nthreads = safe_parse_int_arg(optarg);
            break;
        case 's':
            settings.seed = safe_parse_int_arg(optarg);
            break;
        case 'w':
            settings.workload = safe_parse_int_arg(optarg);
            break;
        default:
            usage();
        }
    }

    settings.type = argv[optind];
    if (!settings.are_valid()) {
        usage();
    }

    if (optind != argc - 1) {
        usage();
    }

    if (settings.type == PQ_CHEAP) {
        kpqbench::cheap<uint32_t, uint32_t> pq;
        ret = bench(&pq, settings);
    } else if (settings.type == PQ_DLSM) {
        kpq::dist_lsm<uint32_t, uint32_t, DEFAULT_RELAXATION> pq;
        ret = bench(&pq, settings);
    } else if (settings.type == PQ_GLOBALLOCK) {
        kpqbench::GlobalLock<uint32_t, uint32_t> pq;
        ret = bench(&pq, settings);
    } else if (settings.type == PQ_KLSM16) {
        kpq::k_lsm<uint32_t, uint32_t, 16> pq;
        ret = bench(&pq, settings);
    } else if (settings.type == PQ_KLSM128) {
        kpq::k_lsm<uint32_t, uint32_t, 128> pq;
        ret = bench(&pq, settings);
    } else if (settings.type == PQ_KLSM256) {
        kpq::k_lsm<uint32_t, uint32_t, 256> pq;
        ret = bench(&pq, settings);
    } else if (settings.type == PQ_KLSM4096) {
        kpq::k_lsm<uint32_t, uint32_t, 4096> pq;
        ret = bench(&pq, settings);
    } else if (settings.type == PQ_LINDEN) {
        kpqbench::Linden pq(kpqbench::Linden::DEFAULT_OFFSET);
        pq.insert(42, 42); /* A hack to avoid segfault on destructor in empty linden queue. */
        ret = bench(&pq, settings);
    } else if (settings.type == PQ_LSM) {
        kpq::LSM<uint32_t> pq;
        ret = bench(&pq, settings);
    } else if (settings.type == PQ_MULTIQ) {
        kpqbench::multiq<uint32_t, uint32_t> pq(settings.nthreads);
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
