#!/usr/bin/env sh

set -eu

cmake -S . -B build
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure

echo "Build and tests completed successfully."
