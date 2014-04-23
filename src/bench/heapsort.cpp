#include <ctime>
#include <getopt.h>
#include <random>

#include "globallock.h"
#include "lsm.h"

constexpr int DEFAULT_SEED = 0;
constexpr int DEFAULT_NELEMS = 1 << 15;

#define PQ_GLOBALLOCK "globallock"
#define PQ_LSM        "lsm"

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
            "       -s: Specifies the value used to seed the random number generator (default = 0)\n"
            "       -n: Specifies the number of elements to sort (default = 2^15)\n"
            "       pq: The data structure to use as the backing priority queue\n"
            "           (one of 'globallock', 'lsm')\n");
    exit(EXIT_FAILURE);
}

static double
timediff_in_s(const struct timespec &start,
              const struct timespec &end)
{
    struct timespec tmp;
    if (end.tv_nsec < start.tv_nsec) {
        tmp.tv_sec = end.tv_sec - start.tv_sec - 1;
        tmp.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
    } else {
        tmp.tv_sec = end.tv_sec - start.tv_sec;
        tmp.tv_nsec = end.tv_nsec - start.tv_nsec;
    }

    return tmp.tv_sec + (double)tmp.tv_nsec / 1000000000.0;
}

static std::vector<uint32_t>
random_array(const struct settings &settings)
{
    std::vector<uint32_t> xs;
    xs.reserve(settings.nelems);

    std::mt19937 gen(settings.seed);
    std::uniform_int_distribution<> rand_int;

    for (int i = 0; i < settings.nelems; i++) {
        xs.push_back(rand_int(gen));
    }

    return xs;
}

template <class T>
static int
bench(T *pq,
      const struct settings &settings)
{
    int ret = 0;

    std::vector<uint32_t> xs = random_array(settings);

    /* Begin benchmark. */
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < settings.nelems; i++) {
        pq->insert(xs[i]);
    }

    for (int i = 0; i < settings.nelems; i++) {
        pq->delete_min(xs[i]);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    /* End benchmark. */

    const double elapsed = timediff_in_s(start, end);
    fprintf(stdout, "%s, %d, %1.8f\n", settings.type.c_str(), settings.nelems, elapsed);

    /* Verify results. */
    for (int i = 1; i < settings.nelems; i++) {
        if (xs[i] < xs[i - 1]) {
            fprintf(stderr, "INVALID RESULTS: xs[%d] < xs[%d]\n", i, i - 1);
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
        kpq::GlobalLock pq;
        ret = bench(&pq, settings);
    } else if (settings.type == PQ_LSM) {
        kpq::LSM<uint32_t> pq;
        ret = bench(&pq, settings);
    } else {
        usage();
    }

    return ret;
}
