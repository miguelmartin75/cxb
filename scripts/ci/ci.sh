#!/usr/bin/env bash
set -euo pipefail

# Ensure submodules are present (Catch2)
git submodule update --init --recursive

# Simple CI script intended to be run inside the Nix devShell (see scripts/ci/flake.nix)

# Allow CI to override compiler via the COMPILER env variable (e.g. gcc or clang)
if [[ -n "${COMPILER:-}" ]]; then
  compilers=("$COMPILER")
else
  compilers=(gcc clang)
fi

# Allow CI to override build type via the BUILD_TYPE env variable (e.g. Debug or Release)
if [[ -n "${BUILD_TYPE:-}" ]]; then
  build_types=("$BUILD_TYPE")
else
  build_types=(Debug Release)
fi

for compiler in "${compilers[@]}"; do
  for build_type in "${build_types[@]}"; do
    echo -e "\n=== Building with ${compiler^^} (${build_type}) ===\n"
    if [[ "$compiler" == "clang" ]]; then
      export CC=clang
      export CXX=clang++
    else
      export CC=gcc
      export CXX=g++
    fi

    build_dir="build/${compiler}-${build_type}"
    rm -rf "$build_dir"
    cmake -B "$build_dir" -DCMAKE_BUILD_TYPE="${build_type}" -DCXB_BUILD_C_API_TESTS=ON
    cmake --build "$build_dir" --config "${build_type}" -j

    pushd "$build_dir" >/dev/null
    ctest --output-on-failure --verbose
    popd >/dev/null
  done
done

echo "All builds and tests succeeded!" 
