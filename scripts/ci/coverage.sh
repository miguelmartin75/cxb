#!/usr/bin/env bash
set -euo pipefail

# Ensure submodules are present
git submodule update --init --recursive

export CC=clang
export CXX=clang++
LLVM_PROFDATA=${LLVM_PROFDATA:-llvm-profdata-18}
LLVM_COV=${LLVM_COV:-llvm-cov-18}

build_dir="build/coverage"
rm -rf "$build_dir"
cmake -B "$build_dir" -DCMAKE_BUILD_TYPE=Debug -DCXB_BUILD_TESTS=ON -DCXB_BUILD_C_API_TESTS=ON \
  -DCMAKE_CXX_FLAGS="-fprofile-instr-generate -fcoverage-mapping" -DCMAKE_C_FLAGS="-fprofile-instr-generate -fcoverage-mapping" -DCMAKE_EXE_LINKER_FLAGS="-fprofile-instr-generate"
cmake --build "$build_dir" --config Debug -j

pushd "$build_dir" >/dev/null
LLVM_PROFILE_FILE="cxb-%p.profraw" ctest --output-on-failure
"$LLVM_PROFDATA" merge cxb-*.profraw -o coverage.profdata
"$LLVM_COV" report $(find . -maxdepth 1 -type f -name 'test_*') -instr-profile=coverage.profdata > coverage.txt
cat coverage.txt
popd >/dev/null
