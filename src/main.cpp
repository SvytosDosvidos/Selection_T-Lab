#include <algorithm>
#include <exception>
#include <iostream>
#include <thread>

#include "graph_builder.h"
#include "pagerank.h"

using namespace std;

int main(int argc, char** argv) {
    if (argc != 2) {
        cerr << "incorrect number of input arguments\n";
        return 1;
    }

    size_t memory_limit = 128ULL * 1024ULL * 1024ULL;
    size_t threads = max(1U, thread::hardware_concurrency());

    try {
        BuildGraph(argv[1], "work", memory_limit, threads);
        PageRankResult result = RunPageRank("work", "pagerank.csv", threads);

        if (!result.converged) {
            cerr << "PageRank did not converge after " << result.iterations
                 << " iterations\n";
            return 2;
        }

        cout << "result written to pagerank.csv\n";
        return 0;
    } catch (const exception& error) {
        cerr << "error: " << error.what() << '\n';
        return 1;
    }
}