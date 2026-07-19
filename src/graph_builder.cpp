#include "graph_builder.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include "csv_reader.h"

using namespace std;
namespace fs = std::filesystem;

bool ReadId(ifstream& file, int32_t& id) {
    file.read(reinterpret_cast<char*>(&id), sizeof(id));

    if (file) {
        return true;
    }

    if (file.eof() && file.gcount() == 0) {
        return false;
    }

    throw runtime_error("can't read temporary file");
}

void SaveIds(const fs::path& file_name, const vector<int32_t>& ids) {
    ofstream file(file_name, ios::binary | ios::trunc);

    if (!file) {
        throw runtime_error("can't create file: " + file_name.string());
    }

    file.write(reinterpret_cast<const char*>(ids.data()),
               static_cast<streamsize>(ids.size() * sizeof(int32_t)));

    if (!file) {
        throw runtime_error("can't write file: " + file_name.string());
    }
}

fs::path SaveRun(vector<int32_t>& ids, const fs::path& run_dir, size_t number) {
    sort(ids.begin(), ids.end());

    size_t unique_count = 0;
    for (int32_t id : ids) {
        if (unique_count == 0 || ids[unique_count - 1] != id) {
            ids[unique_count] = id;
            ++unique_count;
        }
    }

    ids.resize(unique_count);

    fs::path file_name = run_dir / ("run_" + to_string(number) + ".bin");
    SaveIds(file_name, ids);
    ids.clear();
    return file_name;
}

void MergeTwoFiles(const fs::path& left_name, const fs::path& right_name,
                   const fs::path& output_name) {
    ifstream left_file(left_name, ios::binary);
    ifstream right_file(right_name, ios::binary);

    ofstream output(output_name, ios::binary | ios::trunc);
    if (!left_file || !right_file || !output) {
        throw runtime_error("can't open files for merge");
    }

    int32_t left_id = 0, right_id = 0, last_id = 0;
    bool has_left = ReadId(left_file, left_id);
    bool has_right = ReadId(right_file, right_id);
    bool has_last_id = false;

    while (has_left || has_right) {
        int32_t id;

        if (!has_right || (has_left && left_id <= right_id)) {
            id = left_id;
            has_left = ReadId(left_file, left_id);
        } else {
            id = right_id;
            has_right = ReadId(right_file, right_id);
        }

        if (!has_last_id || id != last_id) {
            output.write(reinterpret_cast<const char*>(&id), sizeof(id));
            last_id = id;
            has_last_id = true;
        }
    }

    if (!output) {
        throw runtime_error("can't write file: " + output_name.string());
    }
}

void MergeAllRuns(vector<fs::path> runs, const fs::path& run_dir,
                  const fs::path& output_name) {
    size_t stage = 0;

    while (runs.size() > 1) {
        vector<fs::path> next_runs;

        for (size_t i = 0; i < runs.size(); i += 2) {
            if (i + 1 == runs.size()) {
                next_runs.push_back(runs[i]);
                continue;
            }

            fs::path merged = run_dir / ("merge_" + to_string(stage) + "_" +
                                         to_string(next_runs.size()) + ".bin");

            MergeTwoFiles(runs[i], runs[i + 1], merged);

            error_code error;
            fs::remove(runs[i], error);
            fs::remove(runs[i + 1], error);
            next_runs.push_back(merged);
        }

        runs = move(next_runs);
        ++stage;
    }

    if (runs.empty()) {
        SaveIds(output_name, {});
        return;
    }

    error_code error;
    fs::remove(output_name, error);
    error.clear();
    fs::rename(runs[0], output_name, error);

    if (error) {
        throw runtime_error("can't create file: " + output_name.string());
    }
}

uint64_t FileBytes(const fs::path& file_name) {
    error_code error;
    uint64_t bytes = fs::file_size(file_name, error);

    if (error) {
        throw runtime_error("can't get file size: " + file_name.string());
    }

    return bytes;
}

vector<int32_t> LoadVertices(const fs::path& file_name) {
    uint64_t bytes = FileBytes(file_name);

    if (bytes % sizeof(int32_t) != 0) {
        throw runtime_error("broken vertex file");
    }

    vector<int32_t> vertices(bytes / sizeof(int32_t));
    ifstream file(file_name, ios::binary);

    if (!file) {
        throw runtime_error("can't open vertex file");
    }

    file.read(reinterpret_cast<char*>(vertices.data()), static_cast<streamsize>(bytes));

    if (!file && bytes != 0) {
        throw runtime_error("can't read vertex file");
    }

    return vertices;
}

uint32_t FindVertex(const vector<int32_t>& vertices, int32_t id) {
    auto position = lower_bound(vertices.begin(), vertices.end(), id);

    if (position == vertices.end() || *position != id) {
        throw runtime_error("id not found");
    }

    return static_cast<uint32_t>(position - vertices.begin());
}

void SaveDegrees(const fs::path& file_name, const vector<uint64_t>& degrees) {
    ofstream file(file_name, ios::binary | ios::trunc);

    if (!file) {
        throw runtime_error("can't create degree file");
    }

    file.write(reinterpret_cast<const char*>(degrees.data()),
               static_cast<streamsize>(degrees.size() * sizeof(uint64_t)));

    if (!file) {
        throw runtime_error("can't write degree file");
    }
}

void SaveGraphInfo(const fs::path& work_dir, const GraphInfo& info) {
    ofstream file(work_dir / "graph.info", ios::trunc);

    if (!file) {
        throw runtime_error("can't create graph.info");
    }

    file << info.vertex_count << ' ' << info.edge_count << ' ' << info.partition_count
         << '\n';
}

fs::path VertexFile(const fs::path& work_dir) { return work_dir / "vertices.bin"; }

fs::path DegreeFile(const fs::path& work_dir) { return work_dir / "degrees.bin"; }

fs::path PartFile(const fs::path& work_dir, size_t part) {
    return work_dir / ("edges_" + to_string(part) + ".bin");
}

size_t GetPart(uint32_t vertex, const GraphInfo& info) {
    return static_cast<size_t>(static_cast<uint64_t>(vertex) * info.partition_count /
                               info.vertex_count);
}

GraphInfo ReadGraphInfo(const fs::path& work_dir) {
    ifstream file(work_dir / "graph.info");
    GraphInfo info;

    if (!(file >> info.vertex_count >> info.edge_count >> info.partition_count)) {
        throw runtime_error("can't read graph.info");
    }

    return info;
}

GraphInfo BuildGraph(const fs::path& input_csv, const fs::path& work_dir,
                     size_t memory_limit, size_t threads) {
    fs::create_directories(work_dir);
    fs::path run_dir = work_dir / "runs";
    error_code error;
    fs::remove_all(run_dir, error);
    fs::create_directories(run_dir);

    cout << "collecting vertex ids\n";

    size_t max_ids = max<size_t>(1024, memory_limit / 4 / sizeof(int32_t));
    vector<int32_t> ids;
    ids.reserve(max_ids);
    vector<fs::path> runs;

    CsvReader first_pass(input_csv.string());
    Edge edge{};

    while (first_pass.Read(edge)) {
        ids.push_back(edge.from);
        ids.push_back(edge.to);

        if (ids.size() >= max_ids) {
            runs.push_back(SaveRun(ids, run_dir, runs.size()));
        }
    }

    if (!ids.empty()) {
        runs.push_back(SaveRun(ids, run_dir, runs.size()));
    }

    MergeAllRuns(runs, run_dir, VertexFile(work_dir));

    uint64_t vertex_bytes = FileBytes(VertexFile(work_dir));
    if (vertex_bytes % sizeof(int32_t) != 0) {
        throw runtime_error("broken vertex file");
    }

    uint64_t vertex_count = vertex_bytes / sizeof(int32_t);
    if (vertex_count > numeric_limits<uint32_t>::max()) {
        throw runtime_error("too many vertices");
    }

    uint64_t rank_memory =
        vertex_count * (sizeof(int32_t) + sizeof(uint64_t) + 2 * sizeof(double));

    if (rank_memory > memory_limit * 8 / 10) {
        throw runtime_error(
            "vertex arrays do not fit memory limit; increase memory limit");
    }

    vector<int32_t> vertices = LoadVertices(VertexFile(work_dir));

    GraphInfo info;
    info.vertex_count = vertices.size();

    if (!vertices.empty()) {
        size_t wanted_parts = min<size_t>(64, max<size_t>(1, threads * 4));
        info.partition_count = min(vertices.size(), wanted_parts);
    }

    vector<ofstream> parts;
    parts.reserve(info.partition_count);

    for (size_t i = 0; i < info.partition_count; ++i) {
        parts.emplace_back(PartFile(work_dir, i), ios::binary | ios::trunc);
        if (!parts.back()) {
            throw runtime_error("can't create edge partition");
        }
    }

    vector<uint64_t> degrees(vertices.size(), 0);
    CsvReader second_pass(input_csv.string());

    cout << "writing edge partitions\n";

    while (second_pass.Read(edge)) {
        uint32_t from = FindVertex(vertices, edge.from);
        uint32_t to = FindVertex(vertices, edge.to);
        IndexedEdge indexed_edge{from, to};
        size_t part = GetPart(to, info);

        parts[part].write(reinterpret_cast<const char*>(&indexed_edge),
                          sizeof(indexed_edge));

        if (!parts[part]) {
            throw runtime_error("can't write edge partition");
        }

        ++degrees[from];
        ++info.edge_count;
    }

    for (ofstream& part : parts) {
        part.close();
    }

    SaveDegrees(DegreeFile(work_dir), degrees);
    SaveGraphInfo(work_dir, info);
    fs::remove_all(run_dir, error);

    cout << "vertices: " << info.vertex_count << '\n';
    cout << "edges: " << info.edge_count << '\n';
    cout << "partitions: " << info.partition_count << '\n';

    return info;
}