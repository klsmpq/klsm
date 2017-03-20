/*
 *  Copyright 2017 Kjell Winblad and Jakob Gruber
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
#include <cstdlib>
#include <future>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <random>
#include <thread>
#include <unistd.h>

#include "dist_lsm/dist_lsm.h"
#include "k_lsm/k_lsm.h"
#include "pqs/cppcapq.h"
#include "pqs/globallock.h"
#include "pqs/multiq.h"
#include "pqs/linden.h"
#include "pqs/spraylist.h"
#include "util.h"

constexpr int DEFAULT_NTHREADS   = 1;
constexpr int DEFAULT_RELAXATION = 256;
constexpr int DEFAULT_SEED       = 0;
constexpr int ARRAY_PADDING       = 16;
static std::string DEFAULT_OUTPUT_FILE = "out.txt";

#define PQ_CADM        "cadm"     /* CA-DM in  "The Contention Avoiding Concurrent Priority Queue" LCPC'2016 */
#define PQ_CAIN        "cain"     /* CA-IN in  ... */
#define PQ_CAPQ        "capq"     /* CA-PQ in  ... */
#define PQ_CATREE      "catree"   /* CATree in ... */
#define PQ_DLSM        "dlsm"
#define PQ_GLOBALLOCK  "globallock"
#define PQ_KLSM        "klsm"
#define PQ_KLSM16      "klsm16"
#define PQ_KLSM128     "klsm128"
#define PQ_KLSM256     "klsm256"
#define PQ_KLSM512     "klsm512"
#define PQ_KLSM1024    "klsm1024"
#define PQ_KLSM2048    "klsm2048"
#define PQ_KLSM4096    "klsm4096"
#define PQ_KLSM8192    "klsm8192"
#define PQ_KLSM16384   "klsm16384"
#define PQ_KLSM32768   "klsm32768"
#define PQ_KLSM65536   "klsm65536"
#define PQ_KLSM131072  "klsm131072"
#define PQ_MULTIQC2    "multiqC2"
#define PQ_MULTIQC4    "multiqC4"
#define PQ_MULTIQC8    "multiqC8"
#define PQ_MULTIQC16   "multiqC16"
#define PQ_MULTIQC32   "multiqC32"
#define PQ_MULTIQC64   "multiqC64"
#define PQ_MULTIQC128  "multiqC128"
#define PQ_MULTIQC256  "multiqC256"
#define PQ_LINDEN      "linden"
#define PQ_SPRAYLIST   "spraylist"
/* Hack to support graphs that are badly formatted */
#define IGNORE_NODES_WITH_ID_LARGER_THAN_SIZE 1
/* hwloc does not work on all platforms */
//#define MANUAL_PINNING 1
//#define PAPI 1

#ifdef PAPI
extern "C" {
#include <papi.h>
}
#define G_EVENT_COUNT 2
#define MAX_THREADS 80
int g_events[] = { PAPI_L2_DCA, PAPI_L2_DCM  };
long long g_values[MAX_THREADS][G_EVENT_COUNT] = {0,};
#endif

static hwloc_wrapper hwloc; /**< Thread pinning functionality. */

static std::atomic<bool> start_barrier(false);

struct threads_waiting_to_succeed_pad {
    char pad1[128];
    std::atomic<int> threads_waiting_to_succeed;
    char pad2[128];
};

static threads_waiting_to_succeed_pad wt;

static std::atomic<long> *threads_wait_switches;

struct settings {
    int num_threads;
    std::string graph_file;
    int seed;
    size_t max_generated_random_weight;
    std::string output_file;
    std::string type;
};

struct edge_t {
    size_t target;
    size_t weight;
};

struct vertex_t {
    size_t num_edges;
    std::atomic<size_t> distance;
    edge_t *edges;
};

static void
usage()
{
    fprintf(stderr,
            "USAGE: shortest_paths -i input_file [-n num_threads] [-w end_of_range]\n"
            "                      [-s seed] pq\n"
            "       -i: The input graph file *\n"
            "       -n: Number of threads (default = %d)\n"
            "       -w: Generate random weights between 0 and end_of_range\n"
            "       -s: The random number generator seed (default = %d)\n"
            "       -o: Output file name (default = %s)\n"
            "       pq: The data structure to use as the backing priority queue\n"
            "           (one of '%s', '%s', '%s', '%s', '%s', '%s',\n"
            "                   '%s', '%s', '%s', '%s', '%s', '%s',\n"
            "                   '%s', '%s', '%s', '%s', '%s', '%s',\n"
            "                   '%s', '%s', '%s', '%s', '%s', '%s',\n"
            "                   '%s', '%s', '%s', '%s', '%s')\n"
            "\n"
            "Output:\n"
            "The program output the time used for calculating the shortest paths and\n"
            "the number of nodes processed by the algorithm (separated by a space\n"
            "character) to standard output. Note that the number of nodes processed\n"
            "by the algorithm can be more than the number of nodes in the graph if\n"
            "more than one thread is used.\n"
            "\n"
	    "IMPORTANT: The automatic pinning of threads to hardware threads\n"
	    "using hwloc appears to be broken on some machines. To fix this,\n"
	    "you can do the pinning manually by editing the source code of\n"
	    "file_shortest_paths.cpp (search for the string MANUAL_PINNING).\n"
	    "\n"
            " * Input graph files can either be obtained from\n"
            "http://snap.stanford.edu/data/ or generated with the program\n"
            "generate_random_graph (located in the same folder as this program).\n",
            DEFAULT_NTHREADS,
            DEFAULT_SEED,
            DEFAULT_OUTPUT_FILE.c_str(),
            PQ_CADM, PQ_CAIN, PQ_CAPQ, PQ_CATREE, PQ_DLSM, PQ_GLOBALLOCK,
            PQ_KLSM, PQ_KLSM16, PQ_KLSM128, PQ_KLSM256, PQ_KLSM512,
            PQ_KLSM1024, PQ_KLSM2048, PQ_KLSM4096, PQ_KLSM8192, PQ_KLSM16384,
            PQ_KLSM32768, PQ_KLSM65536, PQ_KLSM131072, PQ_MULTIQC2,
            PQ_MULTIQC4, PQ_MULTIQC8, PQ_MULTIQC16, PQ_MULTIQC32,
            PQ_MULTIQC64, PQ_MULTIQC128, PQ_MULTIQC256, PQ_LINDEN,
            PQ_SPRAYLIST);
    exit(EXIT_FAILURE);
}

static vertex_t *
read_graph(std::string file_path,
           bool generate_weights,
           size_t generate_weights_range_end,
           size_t &number_of_nodes_write_back,
           int seed)
{
    std::ifstream file;
    file.open(file_path);
    if (! file.is_open()) {
        std::cerr << "Could not open file: " << file_path << std::endl;
        std::exit(0);
    }
    size_t tmp_num1;
    size_t tmp_num2;
    std::string tmp_str;
    file >> tmp_str >> tmp_str >> tmp_num1 >> tmp_str >>  tmp_num2;
    number_of_nodes_write_back = tmp_num1;
    size_t n = tmp_num1;
    vertex_t *data = new vertex_t[n];
    size_t *current_edge_index = new size_t[n];
    for (size_t i = 0; i < n; i++) {
        data[i].num_edges = 0;
        data[i].distance.store(std::numeric_limits<size_t>::max(), std::memory_order_relaxed);
        data[i].edges = NULL;
    }
    for (size_t i = 0; i < n; i++) {
        current_edge_index[i] = 0;
    }
    while (!file.eof()) {
        file >> tmp_num1 >> tmp_num2;
#ifdef IGNORE_NODES_WITH_ID_LARGER_THAN_SIZE
        if (tmp_num1 >= n) {
            continue;
        }
        if (tmp_num2 >= n) {
            continue;
        }
#endif
        data[tmp_num1].num_edges++;
    }
    file.close();
    file.open(file_path);
    if (! file.is_open()) {
        std::cerr << "Could not open file: " << file_path << std::endl;
        std::exit(0);
    }
    file >> tmp_str >> tmp_str >> tmp_num1 >> tmp_str >>  tmp_num2;
    std::mt19937 rng;
    rng.seed(seed);
    std::uniform_int_distribution<size_t> rnd_st(0, generate_weights_range_end);
    while (!file.eof()) {
        file >> tmp_num1 >> tmp_num2;
#ifdef IGNORE_NODES_WITH_ID_LARGER_THAN_SIZE
        if (tmp_num1 >= n) {
            continue;
        }
        if (tmp_num2 >= n) {
            continue;
        }
#endif
        if (data[tmp_num1].edges == NULL) {
            data[tmp_num1].edges = new edge_t[data[tmp_num1].num_edges];
        }
        data[tmp_num1].edges[current_edge_index[tmp_num1]].target = tmp_num2;
        if (!generate_weights) {
            data[tmp_num1].edges[current_edge_index[tmp_num1]].weight = 1;
        } else {
            data[tmp_num1].edges[current_edge_index[tmp_num1]].weight = rnd_st(rng);;
        }
        current_edge_index[tmp_num1]++;
    }
    delete[] current_edge_index;
    return data;
}

static void
print_graph(const vertex_t *graph,
            const size_t n,
            std::string out_file)
{
    std::ofstream file(out_file);
    if (!file.is_open()) {
        std::cerr << "Unable to open out file: " << out_file << std::endl;
        std::exit(0);
    }
    for (size_t i = 0; i < n; i++) {
        const vertex_t *v = &graph[i];
        const size_t v_dist = v->distance.load(std::memory_order_relaxed);
        if (v_dist == std::numeric_limits<size_t>::max()) {
            file << i << " -1\n";
        } else {
            file << i << " " << v_dist << "\n";
        }
    }
    file.close();
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
             const int number_of_threads,
             const int thread_id,
             vertex_t *graph,
             unsigned long *nodes_processed_writeback)
{
    unsigned long nodes_processed = 0;
#ifdef MANUAL_PINNING
    int cpu = 4 * (thread_id % 16) + thread_id / 16;
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);

    if (pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset) != 0) {
        printf("error setting affinity to %d\n", (int)cpu);
    }
#else
    hwloc.pin_to_core(thread_id);
#endif

#ifdef PAPI
    int papi = PAPI_start_counters(g_events, G_EVENT_COUNT);
    if (PAPI_OK != papi) {
        std::cout << "Problem starting counters " << papi << ".\n";
    }
#endif

    pq->init_thread(number_of_threads);
    while (!start_barrier.load(std::memory_order_relaxed)) {
        /* Wait. */
    }
    while (true) {
        size_t distance;
        size_t node;
        if (!pq->delete_min(distance, node)) {
            bool success = false;
            for (int i = 0 ; i < 400; i++) {
                if (pq->delete_min(distance, node)) {
                    success = true;
                    break;
                }
                std::this_thread::yield();
            }
            if (!success) {
                /*
                  Quite complex protocol to make sure threads don't
                  quit when there are still nodes to process. It works
                  as follows:

                  A thread sets its slot in threads_wait_switches to 1
                  and increments wt.threads_waiting_to_succeed if it
                  has not succeeded in calling delete_min above. It
                  then tries to call delete_min again and revert the
                  changes to threads_wait_switches, decrements
                  wt.threads_waiting_to_succeed and proceed as normal
                  if the delete_min succeeded this time. Otherwise the
                  thread increments wt.threads_waiting_to_succeed,
                  sets its slot in threads_wait_switches to 2 and goes
                  into a waiting state.

                  A waiting thread waits until one of the following
                  conditions are met:

                  1. wt.threads_waiting_to_succeed is equal to the
                  number of working threads times two. This means that
                  after some time point after which no insert
                  operation has been performed, all threads have
                  called delete_min and no thread has succeeded. This
                  guarantees that there is no more node to process in
                  the priority queue provided that the priority queue
                  has the property described in Theorem 4 in the
                  LCPC'2016 publication "The Contention Avoiding
                  Concurrent Priority Queue". To see why this is the
                  case, check that if an insert happened after the
                  last delete_min operation of any of the waiting
                  threads then the thread performing the insert would
                  have waken up all the waiting threads and decremented
                  wt.threads_waiting_to_succeed so that it could not
                  reach the number of threads times two.

                  2. The thread's slot in threads_wait_switches is set
                  to 0 by a thread that has just inserted an
                  item. Note that the thread that set the slot to 0
                  made sure that wt.threads_waiting_to_succeed counter
                  got decremented by two before setting the slot to 0
                  so it is impossible that
                  wt.threads_waiting_to_succeed reach the number of
                  waiting threads times two if not all threads are in
                  waiting state.
                */
                //Tell other threads that we found nothing in the priority queue
                threads_wait_switches[ ARRAY_PADDING + thread_id ].store(1);
                wt.threads_waiting_to_succeed.fetch_add(1);
                if (! pq->delete_min(distance, node)) {
                    int curr_value2 = wt.threads_waiting_to_succeed.fetch_add(1) + 1;
                    threads_wait_switches[ ARRAY_PADDING + thread_id ].store(2);
                    while (true) {
                        if (curr_value2 == (2 * number_of_threads)) {
                            // All threads have observed that
                            // delete_min failed after some timepoint
                            // after which no insert operation has
                            // been performed. We are done!
                            break;
                        }
                        if (threads_wait_switches[ ARRAY_PADDING + thread_id ].load(std::memory_order_acquire) == 0) {
                            // A thread notified the current thread that something got inserted in the queue
                            // Continue trying to delete_min
                            success = true;
                            break;
                        }
                        std::this_thread::yield();
                        curr_value2 = wt.threads_waiting_to_succeed.load(std::memory_order_acquire);
                    }
                    if (!success) {
                        break;
                    } else {
                        continue;
                    }
                } else {
                    wt.threads_waiting_to_succeed.fetch_sub(1);
                    threads_wait_switches[ ARRAY_PADDING + thread_id ].store(0);
                }
            }
        }
        vertex_t *v = &graph[node];
        const size_t v_dist = v->distance.load(std::memory_order_relaxed);
        if (distance > v_dist) {
            /*Dead node... ignore*/
            continue;
        }
        nodes_processed++;
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
                pq->insert(new_dist, e->target);
                if (wt.threads_waiting_to_succeed.load(std::memory_order_acquire) > 0) {
                    // Notify threads that there is still work to do
                    for (int i = ARRAY_PADDING; i < number_of_threads; i++) {
                        while (true) {
                            long currentValue = threads_wait_switches[i].load(std::memory_order_acquire);
                            if (currentValue == 2) {
                                // If the thread's slot has the value 2, it need to be notified
                                long expected = 2;
                                if (threads_wait_switches[i].compare_exchange_strong(expected, 3)) {
                                    // If we successfully changed the thread's slot to 3,
                                    // we will decrement wt.threads_waiting_to_succeed and notify the thread
                                    wt.threads_waiting_to_succeed.fetch_sub(2);
                                    threads_wait_switches[i].store(0, std::memory_order_release);
                                }
                            } else if (currentValue == 0 || currentValue == 3) {
                                // If the thread's slot has the value 0, it does not need to be notified
                                // If the thread's slot has the value 3, another thread will notify the thread
                                break;
                            }
                            std::this_thread::yield();
                        }
                    }
                }
            }
        }
    }
    *nodes_processed_writeback = nodes_processed;
#ifdef PAPI
    int papi2 = PAPI_read_counters(g_values[thread_id], G_EVENT_COUNT);
    if (PAPI_OK != papi2) {
        std::cout << "Problem reading counters " << papi2 << ".\n";
    }
#endif
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
    size_t number_of_nodes;
    vertex_t *graph = read_graph(settings.graph_file,
                                 settings.max_generated_random_weight != 0,
                                 settings.max_generated_random_weight,
                                 number_of_nodes,
                                 settings.seed);


    /* Our initial node is graph[0]. */

    pq->insert((size_t)0, (size_t)0);

    graph[0].distance.store(0);

    /* Start all threads. */

    std::vector<std::thread> threads(settings.num_threads);
    size_t *number_of_nodes_processed_for_thread =
        new size_t [settings.num_threads];
    wt.threads_waiting_to_succeed = 0;
    for (int i = 0; i < settings.num_threads; i++) {
        threads[i] = std::thread(bench_thread<T>,
                                 pq,
                                 settings.num_threads,
                                 i,
                                 graph,
                                 &number_of_nodes_processed_for_thread[i]);
    }

    /* Begin benchmark. */
    start_barrier.store(true, std::memory_order_relaxed);

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (auto &thread : threads) {
        thread.join();
    }

    size_t total_number_of_nodes_processed = 0;

    for (int i = 0; i < settings.num_threads; i++) {
        total_number_of_nodes_processed =
            total_number_of_nodes_processed +
            number_of_nodes_processed_for_thread[i];
    }
    delete[] number_of_nodes_processed_for_thread;

    clock_gettime(CLOCK_MONOTONIC, &end);
    /* End benchmark. */

    print_graph(graph,
                number_of_nodes,
                settings.output_file);

    const double elapsed = timediff_in_s(start, end);
    fprintf(stdout, "%f %lu", elapsed, total_number_of_nodes_processed);

    delete_graph(graph, number_of_nodes);
    return ret;
}

int
main(int argc,
     char **argv)
{
    int ret = 0;
    struct settings s = { DEFAULT_NTHREADS, "", DEFAULT_SEED, 0, DEFAULT_OUTPUT_FILE, PQ_DLSM};

    int opt;
    while ((opt = getopt(argc, argv, "i:n:w:o:p:s:")) != -1) {
        switch (opt) {
        case 'i':
            errno = 0;
            s.graph_file = optarg;
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
        case 'w':
            errno = 0;
            s.max_generated_random_weight = strtol(optarg, NULL, 0);
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
        case 'o':
            errno = 0;
            s.output_file = optarg;
            if (errno != 0) {
                usage();
            }
            break;
        default:
            usage();
        }
    }

    if (s.graph_file == "") {
        usage();
    }

    if (s.num_threads < 1) {
        usage();
    }


    if (optind != argc - 1) {
        usage();
    }

    s.type = argv[optind];

    // Padding + space for each thread
    threads_wait_switches = new std::atomic<long>[ARRAY_PADDING * 2 + s.num_threads];
    for (int i = ARRAY_PADDING; i < s.num_threads; i++) {
        threads_wait_switches[i].store(0);
    }

#ifdef PAPI
    if (PAPI_VER_CURRENT != PAPI_library_init(PAPI_VER_CURRENT)) {
        std::cout << ("PAPI_library_init error.\n");
        return 0;
    }
    if (PAPI_OK != PAPI_query_event(PAPI_L2_DCA)) {
        std::cout << ("Cannot count PAPI_L2_DCA.\n");
    }
    if (PAPI_OK != PAPI_query_event(PAPI_L2_DCM)) {
        std::cout << ("Cannot count PAPI_L2_DCM.");
    }
#endif


    if (s.type == PQ_CADM) {
        kpqbench::CPPCAPQ<true, false, true> pq;
        ret = bench(&pq, s);
    } else if (s.type == PQ_CAIN) {
        kpqbench::CPPCAPQ<false, true, true> pq;
        ret = bench(&pq, s);
    } else if (s.type == PQ_CAPQ) {
        kpqbench::CPPCAPQ<true, true, true> pq;
        ret = bench(&pq, s);
    } else if (s.type == PQ_CATREE) {
        kpqbench::CPPCAPQ<false, false, true> pq;
        ret = bench(&pq, s);
    } else if (s.type == PQ_DLSM) {
        kpq::dist_lsm<size_t, size_t, DEFAULT_RELAXATION> pq;
        ret = bench(&pq, s);
    } else if (s.type == PQ_KLSM) {
        kpq::k_lsm<size_t, size_t, DEFAULT_RELAXATION> pq;
        ret = bench(&pq, s);
    } else if (s.type == PQ_KLSM16) {
        kpq::k_lsm<size_t, size_t, 16> pq;
        ret = bench(&pq, s);
    } else if (s.type == PQ_KLSM128) {
        kpq::k_lsm<size_t, size_t, 128> pq;
        ret = bench(&pq, s);
    } else if (s.type == PQ_KLSM256) {
        kpq::k_lsm<size_t, size_t, 256> pq;
        ret = bench(&pq, s);
    } else if (s.type == PQ_KLSM512) {
        kpq::k_lsm<size_t, size_t, 512> pq;
        ret = bench(&pq, s);
    } else if (s.type == PQ_KLSM1024) {
        kpq::k_lsm<size_t, size_t, 1024> pq;
        ret = bench(&pq, s);
    } else if (s.type == PQ_KLSM2048) {
        kpq::k_lsm<size_t, size_t, 2048> pq;
        ret = bench(&pq, s);
    } else if (s.type == PQ_KLSM4096) {
        kpq::k_lsm<size_t, size_t, 4096> pq;
        ret = bench(&pq, s);
    } else if (s.type == PQ_KLSM8192) {
        kpq::k_lsm<size_t, size_t, 8192> pq;
        ret = bench(&pq, s);
    } else if (s.type == PQ_KLSM16384) {
        kpq::k_lsm<size_t, size_t, 16384> pq;
        ret = bench(&pq, s);
    } else if (s.type == PQ_KLSM32768) {
        kpq::k_lsm<size_t, size_t, 32768> pq;
        ret = bench(&pq, s);
    } else if (s.type == PQ_KLSM65536) {
        kpq::k_lsm<size_t, size_t, 65536> pq;
        ret = bench(&pq, s);
    } else if (s.type == PQ_KLSM131072) {
        kpq::k_lsm<size_t, size_t, 131072> pq;
        ret = bench(&pq, s);
    } else if (s.type == PQ_GLOBALLOCK) {
        kpqbench::GlobalLock<size_t, size_t> pq;
        ret = bench(&pq, s);
    } else if (s.type == PQ_LINDEN) {
        kpqbench::Linden pq(kpqbench::Linden::DEFAULT_OFFSET);
        ret = bench(&pq, s);
    } else if (s.type == PQ_MULTIQC2) {
        kpqbench::multiq<size_t, size_t, 2> pq(s.num_threads);
        ret = bench(&pq, s);
    } else if (s.type == PQ_MULTIQC4) {
        kpqbench::multiq<size_t, size_t, 4> pq(s.num_threads);
        ret = bench(&pq, s);
    } else if (s.type == PQ_MULTIQC8) {
        kpqbench::multiq<size_t, size_t, 8> pq(s.num_threads);
        ret = bench(&pq, s);
    } else if (s.type == PQ_MULTIQC16) {
        kpqbench::multiq<size_t, size_t, 16> pq(s.num_threads);
        ret = bench(&pq, s);
    } else if (s.type == PQ_MULTIQC32) {
        kpqbench::multiq<size_t, size_t, 32> pq(s.num_threads);
        ret = bench(&pq, s);
    } else if (s.type == PQ_MULTIQC64) {
        kpqbench::multiq<size_t, size_t, 64> pq(s.num_threads);
        ret = bench(&pq, s);
    }  else if (s.type == PQ_MULTIQC128) {
        kpqbench::multiq<size_t, size_t, 128> pq(s.num_threads);
        ret = bench(&pq, s);
    }  else if (s.type == PQ_MULTIQC256) {
        kpqbench::multiq<size_t, size_t, 256> pq(s.num_threads);
        ret = bench(&pq, s);
    } else if (s.type == PQ_SPRAYLIST) {
        kpqbench::spraylist pq(s.num_threads);
        ret = bench(&pq, s);
    } else {
        usage();
        return ret;
    }
#ifdef PAPI
    long total_L2_cache_accesses = 0;
    long total_L2_cache_misses = 0;
    int k = 0;
    for (k = 0; k <  s.num_threads; k++) {
        total_L2_cache_accesses += g_values[k][0];
        total_L2_cache_misses += g_values[k][1];
    }
    printf(" %ld %ld", total_L2_cache_accesses, total_L2_cache_misses);
#endif
    printf("\n");
    return ret;
}
