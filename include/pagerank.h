#pragma once

#include <filesystem>

using std::filesystem::path;

struct PageRankResult {
    size_t iterations = 0;
    double error = 0.0;
    bool converged = false;
};

PageRankResult RunPageRank(const path& work_dir, const path& output_csv, size_t threads,
                           double damping = 0.85, double epsilon = 1e-8,
                           size_t max_iterations = 200);