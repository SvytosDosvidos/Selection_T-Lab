#include "csv_reader.h"

#include <limits>
#include <stdexcept>
#include <string>

using namespace std;

string DeleteSpaces(const string& text) {
    size_t begin = text.find_first_not_of(" \t\r\n");

    if (begin == string::npos) {
        return {};
    }

    size_t end = text.find_last_not_of(" \t\r\n");
    return text.substr(begin, end - begin + 1);
}

bool ParseId(const string& text, int32_t& id) {
    try {
        size_t used = 0;
        long long value = stoll(text, &used);

        if (used != text.size() || value < numeric_limits<int32_t>::min() ||
            value > numeric_limits<int32_t>::max()) {
            return false;
        }

        id = static_cast<int32_t>(value);
        return true;
    } catch (const exception&) {
        return false;
    }
}

CsvReader::CsvReader(const string& file_name)
    : file_(file_name), file_name_(file_name) {
    if (!file_) {
        throw runtime_error("can't open input data: " + file_name);
    }
}

bool CsvReader::Read(Edge& edge) {
    while (getline(file_, line_)) {
        ++line_number_;

        string line = DeleteSpaces(line_);
        if (line.empty()) {
            continue;
        }

        size_t first_comma = line.find(',');
        if (first_comma == string::npos) {
            throw runtime_error("incorrect input data " + file_name_ + "in the line " +
                                line_);
        }

        string from = DeleteSpaces(line.substr(0, first_comma));
        string to = DeleteSpaces(line.substr(first_comma + 1));

        if (first_line_) {
            first_line_ = false;
            if (from == "from" && to == "to") {
                continue;
            }
        }

        if (!ParseId(from, edge.from) || !ParseId(to, edge.to)) {
            throw runtime_error("incorrect input data " + file_name_ + "in the line " +
                                line_);
        }

        return true;
    }

    if (!file_.eof()) {
        throw runtime_error("can't read input data: " + file_name_);
    }

    return false;
}