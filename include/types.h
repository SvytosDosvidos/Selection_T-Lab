#pragma once

#include <stdint.h>

struct Edge {
    int32_t from;
    int32_t to;
};

struct IndexedEdge {
    uint32_t from;
    uint32_t to;
};