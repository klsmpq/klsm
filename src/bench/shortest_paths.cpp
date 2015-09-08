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

#include "pqs/globallock.h"
#include "pqs/multiq.h"
#include "dist_lsm/dist_lsm.h"
#include "k_lsm/k_lsm.h"
#include "util.h"

constexpr int DEFAULT_NNODES     = 8192;
constexpr int DEFAULT_NTHREADS   = 1;
constexpr double DEFAULT_EDGE_P  = 0.5;
constexpr int DEFAULT_RELAXATION = 256;
constexpr int DEFAULT_SEED       = 0;

#define PQ_DLSM       "dlsm"
#define PQ_GLOBALLOCK "globallock"
#define PQ_KLSM       "klsm"
#define PQ_MULTIQ     "multiq"

static hwloc_wrapper hwloc; /**< Thread pinning functionality. */

static std::atomic<bool> start_barrier(false);
static std::atomic<int> num_tasks(0);

struct settings {
    int num_nodes;
    int num_threads;
    double edge_probability;
    int seed;
    std::string type;
};

struct edge_t {
    size_t target;
    size_t weight;
};

struct vertex_t {
    edge_t *edges;
    size_t num_edges;
    std::atomic<size_t> distance;
};

struct task_t {
    task_t(vertex_t *v,
           const size_t distance) : v(v), distance(distance)
    {
        num_tasks.fetch_add(1, std::memory_order_relaxed);
    }

    ~task_t()
    {
        num_tasks.fetch_sub(1, std::memory_order_relaxed);
    }

    vertex_t *v;
    const size_t distance;
};

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
            "           (one of '%s', %s', '%s', '%s')\n",
            DEFAULT_NNODES,
            DEFAULT_NTHREADS,
            DEFAULT_EDGE_P,
            DEFAULT_SEED,
            PQ_DLSM, PQ_GLOBALLOCK, PQ_KLSM, PQ_MULTIQ);
    exit(EXIT_FAILURE);
}

static vertex_t *
generate_graph(const size_t n,
               const int seed,
               const double p)
{
    vertex_t *data = new vertex_t[n];

    std::mt19937 rng;
    rng.seed(seed);
    std::uniform_real_distribution<float> rnd_f(0.0, 1.0);
    std::uniform_int_distribution<size_t> rnd_st(1, std::numeric_limits<int>::max());

    std::vector<edge_t> *edges = new std::vector<edge_t>[n];
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = i + 1; j < n; ++j) {
            if (rnd_f(rng) < p) {
                edge_t e;
                e.target = j;
                e.weight = rnd_st(rng);
                edges[i].push_back(e);
                e.target = i;
                edges[j].push_back(e);
            }
        }
        data[i].num_edges = edges[i].size();
        if (edges[i].size() > 0) {
            data[i].edges = new edge_t[edges[i].size()];
            for (size_t j = 0; j < edges[i].size(); ++j) {
                data[i].edges[j] = edges[i][j];
            }
        } else {
            data[i].edges = NULL;
        }
        data[i].distance = std::numeric_limits<size_t>::max();
    }

    data[0].distance = 0;
    delete[] edges;

    return data;
}

static void
verify_graph(const vertex_t *graph,
             const size_t n)
{
    for (size_t i = 0; i < n; i++) {
        const vertex_t *v = &graph[i];
        const size_t v_dist = v->distance.load(std::memory_order_relaxed);

        for (size_t j = 0; j < v->num_edges; j++) {
            const edge_t *e = &v->edges[j];
            const size_t new_dist = v_dist + e->weight;

            const vertex_t *w = &graph[e->target];
            const size_t w_dist = w->distance.load(std::memory_order_relaxed);

            assert(new_dist >= w_dist), (void)new_dist, (void)w_dist;
        }
    }
}

static void
delete_graph(vertex_t *data,
             const size_t n)
{
    for (size_t i = 0; i < n; ++i) {
        if (data[i].edges != NULL) {
            delete[] data[i].edges;
        }
    }
    delete[] data;
}

template <class T>
static void
bench_thread(T *pq,
             const int thread_id,
             vertex_t *graph)
{
    hwloc.pin_to_core(thread_id);

    while (!start_barrier.load(std::memory_order_relaxed)) {
        /* Wait. */
    }

    while (num_tasks.load(std::memory_order_relaxed) > 0) {
        task_t *task;
        if (!pq->delete_min(task)) {
            continue;
        }

        const vertex_t *v = task->v;
        const size_t v_dist = v->distance.load(std::memory_order_relaxed);

        if (task->distance > v_dist) {
            delete task;
            continue;
        }

        for (size_t i = 0; i < v->num_edges; i++) {
            const edge_t *e = &v->edges[i];
            const size_t new_dist = v_dist + e->weight;

            vertex_t *w = &graph[e->target];
            size_t w_dist = w->distance.load(std::memory_order_relaxed);

            if (new_dist >= w_dist) {
                continue;
            }

            bool dist_updated;
            do {
                dist_updated = w->distance.compare_exchange_strong(w_dist, new_dist,
                               std::memory_order_relaxed);
            } while (!dist_updated && w_dist > new_dist);

            if (dist_updated) {
                pq->insert(new_dist, new task_t(w, new_dist));
            }
        }

        delete task;
    }
}

template <class T>
static int
bench(T *pq,
      const struct settings &settings)
{
    if (settings.num_threads > 1 && !pq->supports_concurrency()) {
        fprintf(stderr, "The given data structure does not support concurrency.\n");
        return -1;
    }

    int ret = 0;
    vertex_t *graph = generate_graph(settings.num_nodes, settings.seed, settings.edge_probability);

    /* Our initial node is graph[0]. */

    pq->insert(0, new task_t(&graph[0], 0));

    /* Start all threads. */

    std::vector<std::thread> threads(settings.num_threads);
    for (int i = 0; i < settings.num_threads; i++) {
        threads[i] = std::thread(bench_thread<T>, pq, i, graph);
    }

    /* Begin benchmark. */
    start_barrier.store(true, std::memory_order_relaxed);

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (auto &thread : threads) {
        thread.join();
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    /* End benchmark. */

    assert(num_tasks.load(std::memory_order_relaxed) == 0);
    verify_graph(graph, settings.num_nodes);

    const double elapsed = timediff_in_s(start, end);
    fprintf(stdout, "%f\n", elapsed);

    delete_graph(graph, settings.num_nodes);
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

    if (s.type == PQ_DLSM) {
        kpq::dist_lsm<uint32_t, task_t *, DEFAULT_RELAXATION> pq;
        ret = bench(&pq, s);
    } else if (s.type == PQ_KLSM) {
        kpq::k_lsm<uint32_t, task_t *, DEFAULT_RELAXATION> pq;
        ret = bench(&pq, s);
    } else if (s.type == PQ_GLOBALLOCK) {
        kpqbench::GlobalLock<uint32_t, task_t *> pq;
        ret = bench(&pq, s);
    } else if (s.type == PQ_MULTIQ) {
        kpqbench::multiq<uint32_t, task_t *> pq(s.num_threads);
        ret = bench(&pq, s);
    } else {
        usage();
    }

    return ret;
}
