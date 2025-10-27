#!/usr/bin/env bash
set -euo pipefail

# Ensure submodules are present
git submodule update --init --recursive

export CC=clang
export CXX=clang++

# Prefer unversioned llvm tools, but fall back to versioned ones
detect_tool() {
  if command -v "$1" >/dev/null 2>&1; then
    printf '%s' "$1"
    return
  fi
  if command -v "$2" >/dev/null 2>&1; then
    printf '%s' "$2"
    return
  fi
  printf '%s' "$1"
}

LLVM_PROFDATA=${LLVM_PROFDATA:-$(detect_tool llvm-profdata llvm-profdata-18)}
LLVM_COV=${LLVM_COV:-$(detect_tool llvm-cov llvm-cov-18)}

build_dir="build/coverage"
rm -rf "$build_dir"
cmake -B "$build_dir" -DCMAKE_BUILD_TYPE=Debug -DCXB_BUILD_TESTS=ON -DCXB_BUILD_C_API_TESTS=ON \
  -DCMAKE_CXX_FLAGS="-fprofile-instr-generate -fcoverage-mapping" -DCMAKE_C_FLAGS="-fprofile-instr-generate -fcoverage-mapping" -DCMAKE_EXE_LINKER_FLAGS="-fprofile-instr-generate"
cmake --build "$build_dir" --config Debug -j

pushd "$build_dir" >/dev/null
LLVM_PROFILE_FILE="cxb-%p.profraw" ctest --output-on-failure
"$LLVM_PROFDATA" merge cxb-*.profraw -o coverage.profdata
"$LLVM_COV" report $(find . -maxdepth 1 -type f -name 'test_*') \
  -instr-profile=coverage.profdata \
  -ignore-filename-regex='(deps/|tests/|^build/)' > coverage.txt
cat coverage.txt
coverage_pct=$(grep -E '^TOTAL' coverage.txt | awk '{print $(NF-3)}' | tr -d '%')
coverage_int=${coverage_pct%.*}
if [ "${coverage_int:-0}" -ge 90 ]; then
  color="brightgreen"
elif [ "${coverage_int:-0}" -ge 75 ]; then
  color="yellow"
else
  color="red"
fi
printf '{\n  "schemaVersion": 1,\n  "label": "coverage",\n  "message": "%s%%",\n  "color": "%s"\n}\n' "$coverage_pct" "$color" > coverage.json
cat coverage.json
cat <<'EOF' > .gitignore
*
!coverage.txt
!coverage.json
!.gitignore
EOF
popd >/dev/null
