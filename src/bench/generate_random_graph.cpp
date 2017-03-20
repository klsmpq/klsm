/*
 *  Copyright 2017 Jakob Gruber and Kjell Winblad
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
#include <iostream>

using namespace std;
constexpr double DEFAULT_EDGE_P  = 0.5;
constexpr int DEFAULT_SEED       = 0;

struct edge_t {
    size_t target;
    size_t weight;
};

struct vertex_t {
    edge_t *edges;
    size_t num_edges;
    std::atomic<size_t> distance;
};

static vertex_t *
generate_graph(const size_t n,
               const int seed,
               const double p,
               size_t &num_edges_writeback)
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
                num_edges_writeback += 2;
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
usage()
{
    fprintf(stderr,
            "USAGE: generate_random_graph [-n number_of_nodes] [-p edge_probability]\n"
            "       -n: Number of nodes in generated graph\n"
            "       -p: Probability of edge between two nodes\n"
            "\n"
            "This program can be used to generate random graphs suitable as\n"
            "input to the file_shortest_paths\n"
            "benchmark (src/bench/file_shortest_paths.cpp). Other graphs that\n"
            "are suitable as input for that benchmark can be found at:\n"
            "http://snap.stanford.edu/data/ .\n");
    exit(EXIT_FAILURE);
}

int
main(int argc,
     char **argv)
{
    int opt;
    size_t number_of_nodes = 1000;
    double edge_probability = 0.5;
    size_t num_edges = 0;
    while ((opt = getopt(argc, argv, "n:p:")) != -1) {
        switch (opt) {
        case 'n':
            errno = 0;
            number_of_nodes = strtoul(optarg, NULL, 0);
            if (errno != 0) {
                usage();
            }
            break;
        case 'p':
            errno = 0;
            edge_probability = strtod(optarg, NULL);
            if (errno != 0) {
                usage();
            }
            break;
        default:
            usage();
        }
    }

    if (optind != argc - 1) {
        usage();
    }

    vertex_t *nodes = generate_graph(number_of_nodes,
                                     0,
                                     edge_probability,
                                     num_edges);
    std::cout << "# Nodes: " << number_of_nodes << " Edges: " << num_edges << std::endl;
    // Print the generated graph to stdout
    for (size_t i; i < number_of_nodes ; i++) {
        for (size_t k = 0; k < nodes[i].num_edges; k++) {
            std::cout << i << "\t" << nodes[i].edges[k].target << std::endl;
        }
    }
    return 0;
}
