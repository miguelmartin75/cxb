#!/usr/bin/env bash
set -euo pipefail

# Simple CI script intended to be run inside the Nix devShell (see ci/flake.nix)

# Allow CI to override compiler via the COMPILER env variable (e.g. gcc or clang)
if [[ -n "${COMPILER:-}" ]]; then
  compilers=("$COMPILER")
else
  compilers=(gcc clang)
fi

build_types=(Debug Release)

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

    build_dir="build-${compiler}-${build_type}"
    cmake -B "$build_dir" -DCMAKE_BUILD_TYPE="${build_type}" -DBUILD_C_API_TESTS=ON
    cmake --build "$build_dir" --config "${build_type}"

    pushd "$build_dir" >/dev/null
    ctest --output-on-failure --verbose
    popd >/dev/null
  done
done

echo "All builds and tests succeeded!" 