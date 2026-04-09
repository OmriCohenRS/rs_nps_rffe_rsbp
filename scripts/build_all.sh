#!/bin/sh
set -e

REPO_ROOT=$(git rev-parse --show-toplevel)
cd "$REPO_ROOT"

cmake -B build -S .
cmake --build build -j