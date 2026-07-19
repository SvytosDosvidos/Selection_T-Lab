#pragma once

#include <stdint.h>

#include <filesystem>

using std::filesystem::path;

struct GraphInfo {
    uint64_t vertex_count = 0;
    uint64_t edge_count = 0;
    size_t partition_count = 0;
};

GraphInfo BuildGraph(const path& input_csv, const path& work_dir, size_t memory_limit,
                     size_t threads);

GraphInfo ReadGraphInfo(const path& work_dir);

path VertexFile(const path& work_dir);
path DegreeFile(const path& work_dir);
path PartFile(const path& work_dir, size_t part);

size_t GetPart(uint32_t vertex, const GraphInfo& info);