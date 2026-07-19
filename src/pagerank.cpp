#include "pagerank.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>

#include "graph_builder.h"
#include "types.h"

using namespace std;
namespace fs = std::filesystem;

template <class T>
vector<T> LoadArray(const fs::path& file_name, uint64_t count) {
    vector<T> values(count);
    ifstream file(file_name, ios::binary);

    if (!file) {
        throw runtime_error("cannot open file: " + file_name.string());
    }

    file.read(reinterpret_cast<char*>(values.data()),
              static_cast<streamsize>(values.size() * sizeof(T)));

    if (!file && !values.empty()) {
        throw runtime_error("cannot read file: " + file_name.string());
    }

    return values;
}

bool ReadPart(const fs::path& work_dir, const GraphInfo& info,
              const vector<double>& rank, const vector<uint64_t>& degrees,
              vector<double>& next, size_t part, uint64_t& edge_count) {
    ifstream file(PartFile(work_dir, part), ios::binary);
    if (!file) {
        return false;
    }

    IndexedEdge edge{};

    while (file.read(reinterpret_cast<char*>(&edge), sizeof(edge))) {
        if (edge.from >= rank.size() || edge.to >= next.size() ||
            degrees[edge.from] == 0) {
            return false;
        }

        if (GetPart(edge.to, info) != part) {
            return false;
        }

        next[edge.to] += rank[edge.from] / static_cast<double>(degrees[edge.from]);
        ++edge_count;
    }

    return file.eof() && file.gcount() == 0;
}

bool ProcessParts(const fs::path& work_dir, const GraphInfo& info,
                  const vector<double>& rank, const vector<uint64_t>& degrees,
                  vector<double>& next, size_t thread_count) {
    atomic<size_t> next_part = 0;
    atomic<bool> ok = true;
    vector<uint64_t> edge_counts(info.partition_count, 0);

    auto worker = [&]() {
        while (ok.load()) {
            size_t part = next_part.fetch_add(1);

            if (part >= info.partition_count) {
                return;
            }

            if (!ReadPart(work_dir, info, rank, degrees, next, part,
                          edge_counts[part])) {
                ok.store(false);
                return;
            }
        }
    };

    size_t worker_count = min(thread_count, info.partition_count);
    vector<thread> workers;
    workers.reserve(worker_count);

    for (size_t i = 0; i < worker_count; ++i) {
        workers.emplace_back(worker);
    }

    for (thread& worker_thread : workers) {
        worker_thread.join();
    }

    uint64_t total_edges = 0;
    for (uint64_t count : edge_counts) {
        total_edges += count;
    }

    return ok.load() && total_edges == info.edge_count;
}

void SaveResult(const fs::path& output_csv, const vector<int32_t>& vertices,
                const vector<double>& rank) {
    ofstream file(output_csv, ios::trunc);

    if (!file) {
        throw runtime_error("cannot create output CSV");
    }

    file << "vertex,rank\n" << setprecision(17);

    for (size_t i = 0; i < vertices.size(); ++i) {
        file << vertices[i] << ',' << rank[i] << '\n';
    }

    if (!file) {
        throw runtime_error("cannot write output CSV");
    }
}

PageRankResult RunPageRank(const fs::path& work_dir, const fs::path& output_csv,
                           size_t thread_count, double damping, double epsilon,
                           size_t max_iterations) {
    if (thread_count == 0 || max_iterations == 0) {
        throw runtime_error("threads and iterations must be positive");
    }

    if (damping <= 0.0 || damping >= 1.0) {
        throw runtime_error("damping must be between 0 and 1");
    }

    if (epsilon <= 0.0) {
        throw runtime_error("epsilon must be positive");
    }

    GraphInfo info = ReadGraphInfo(work_dir);
    vector<int32_t> vertices =
        LoadArray<int32_t>(VertexFile(work_dir), info.vertex_count);
    vector<uint64_t> degrees =
        LoadArray<uint64_t>(DegreeFile(work_dir), info.vertex_count);
    PageRankResult result;

    if (info.vertex_count == 0) {
        SaveResult(output_csv, vertices, {});
        result.converged = true;
        return result;
    }

    double initial_rank = 1.0 / static_cast<double>(info.vertex_count);
    vector<double> rank(info.vertex_count, initial_rank);
    vector<double> next(info.vertex_count, 0.0);

    for (size_t iteration = 1; iteration <= max_iterations; ++iteration) {
        double dangling_rank = 0.0;

        for (size_t i = 0; i < degrees.size(); ++i) {
            if (degrees[i] == 0) {
                dangling_rank += rank[i];
            }
        }

        double base_rank = (1.0 - damping + damping * dangling_rank) /
                           static_cast<double>(info.vertex_count);
        fill(next.begin(), next.end(), 0.0);

        if (!ProcessParts(work_dir, info, rank, degrees, next, thread_count)) {
            throw runtime_error("cannot process edge partitions");
        }

        double error = 0.0;
        double sum = 0.0;

        for (size_t i = 0; i < next.size(); ++i) {
            next[i] = base_rank + damping * next[i];
            error += abs(next[i] - rank[i]);
            sum += next[i];
        }

        rank.swap(next);
        result.iterations = iteration;
        result.error = error;

        cout << "iteration " << iteration << ", error=" << setprecision(10) << error
             << ", sum=" << sum << '\n';

        if (error <= epsilon) {
            result.converged = true;
            break;
        }
    }

    SaveResult(output_csv, vertices, rank);
    return result;
}