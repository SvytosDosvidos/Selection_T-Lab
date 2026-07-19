#pragma once

#include "types.h"

#include <fstream>
#include <string>

using std::ifstream;
using std::string;

class CsvReader {
public:
    explicit CsvReader(const string& file_name);
    bool Read(Edge& edge);
private:
    ifstream file_;
    string file_name_;
    string line_;
    size_t line_number_ = 0;
    bool first_line_ = true;
};