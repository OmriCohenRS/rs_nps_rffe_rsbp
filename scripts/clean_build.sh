#!/bin/sh
set -e

REPO_ROOT=$(git rev-parse --show-toplevel)
cd "$REPO_ROOT"

rm -rf build

find kernel/drivers/apio16/ -maxdepth 1 -type f \
! -name "CMakeLists.txt" \
! -name "Makefile" \
! -name "apio16.c" \
-delete