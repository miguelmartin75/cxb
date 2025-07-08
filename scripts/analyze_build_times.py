#!/usr/bin/env python3
"""
Script to analyze build times using Clang's -ftime-trace feature.
Updates README.md with current header compile time metrics.
"""

import argparse
import json
import math
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path
from collections import defaultdict
from typing import Dict, List, Optional, Tuple


# --------------------------------------------------------------------------------------
# Constants
# --------------------------------------------------------------------------------------

# Sub-set of C++23 standard library headers. Extend as desired.
STD_HEADERS: List[str] = [
    "algorithm", "array", "atomic", "bitset", "chrono", "condition_variable",
    "deque", "exception", "filesystem", "forward_list", "functional", "future",
    "iomanip", "ios", "iosfwd", "iostream", "istream", "iterator", "limits",
    "list", "map", "memory", "mutex", "new", "numeric", "optional", "ostream",
    "queue", "random", "ratio", "regex", "scoped_allocator", "set", "shared_mutex",
    "sstream", "stack", "stdexcept", "streambuf", "string", "string_view",
    "system_error", "thread", "tuple", "type_traits", "typeindex", "typeinfo",
    "unordered_map", "unordered_set", "utility", "valarray", "variant", "vector",
]

# Relative path of the markdown file storing parse-time benchmarks for std headers.
PARSE_TIMES_MD = Path(__file__).parent.parent / "benchmarks" / "PARSE_TIMES.md"


def run_command(cmd: List[str], cwd: Optional[Path] = None) -> Tuple[int, str, str]:
    try:
        result = subprocess.run(
            cmd, 
            capture_output=True, 
            text=True, 
            cwd=cwd,
            check=False
        )
        return result.returncode, result.stdout, result.stderr
    except Exception as e:
        return -1, "", str(e)


def clean_build_dir(build_dir: Path) -> None:
    if build_dir.exists():
        shutil.rmtree(build_dir)
    build_dir.mkdir(parents=True, exist_ok=True)


def configure_cmake(build_dir: Path, source_dir: Path) -> bool:
    """Configure CMake with Clang and -ftime-trace."""
    cmd = [
        "cmake", 
        "-B", str(build_dir),
        "-S", str(source_dir),
        "-DCMAKE_BUILD_TYPE=Debug",
        "-DCMAKE_C_COMPILER=clang",
        "-DCMAKE_CXX_COMPILER=clang++",
        "-DBUILD_C_API_TESTS=ON"
    ]
    
    exit_code, _, e = run_command(cmd)
    if exit_code != 0:
        sys.stderr.write(f"CMake configuration failed:\n{e}")
        return False
    
    return True


def build_project(build_dir: Path) -> bool:
    cmd = ["cmake", "--build", str(build_dir)]
    exit_code, _, e = run_command(cmd)
    if exit_code != 0:
        sys.stderr.write(f"Build failed:\n{e}")
        return False
    
    return True


def find_trace_files(build_dir: Path) -> List[Path]:
    result = []
    for root, dirs, files in os.walk(build_dir):
        for file in files:
          if file.endswith('.json') and "compile_commands" not in file:
              result.append(Path(root) / file)
    return result


def parse_trace_file(trace_file: Path) -> Dict[str, float]:
    """Extract individual header parse durations from a single ftime-trace JSON file.

    The Clang ftime-trace format stores timing information in the ``traceEvents``
    array.  For header parsing we need to consider two possible encodings:

    1. Complete events (``ph == 'X'``) where the duration is stored directly in
       the event with the ``dur`` field.
    2. Begin/End pairs (``ph == 'B'`` / ``ph == 'E'``) where the duration must
       be calculated from the timestamps.

    In both cases, the header path lives in ``event['args']['file']`` (newer
    Clang versions) or ``event['args']['detail']`` (older Clang versions).
    """
    with open(trace_file, "r") as f:
        data = json.load(f)

    header_times: Dict[str, float] = {}

    # Recognise both uppercase (spec) and lowercase (observed in some clang
    # builds) phase markers.
    PH_BEGIN = {"B", "b"}
    PH_END = {"E", "e"}

    PARSE_EVENT_NAMES = {
        "ParseFile",
        "ParseClass",
        "ParseTemplate",
        "InstantiateFunction",
        "InstantiateClass",
    }

    # First pass: single-event duration (ph == 'X')
    for ev in data.get("traceEvents", []):
        if ev.get("ph") != "X":
            continue
        ev_name = ev.get("name", "")
        if ev_name != "Source" and ev_name not in PARSE_EVENT_NAMES:
            continue
        args = ev.get("args", {})
        header_path = args.get("file") or args.get("detail")
        if not header_path:
            continue
        dur_ms = ev.get("dur", 0) / 1000.0
        header_times[header_path] = max(header_times.get(header_path, 0.0), dur_ms)

    # Second pass: begin/end paired events (ph in B/E or b/e)
    open_events: Dict[int, List[Tuple[str, int]]] = defaultdict(list)
    for ev in data.get("traceEvents", []):
        ph = ev.get("ph")
        if ph not in PH_BEGIN.union(PH_END):
            continue
        name = ev.get("name", "")
        if name != "Source":
            continue
        tid = ev.get("tid", 0)
        if ph in PH_BEGIN:
            args = ev.get("args", {})
            header_path = args.get("file") or args.get("detail")
            if not header_path:
                continue
            open_events[tid].append((header_path, ev.get("ts", 0)))
        elif ph in PH_END and open_events[tid]:
            header_path, start_ts = open_events[tid].pop()
            dur_ms = (ev.get("ts", 0) - start_ts) / 1000.0
            header_times[header_path] = max(header_times.get(header_path, 0.0), dur_ms)

    return header_times


def analyze_parse_times(build_dir: Path, headers: List[str]) -> Dict[str, Dict[str, float]]:
    """Aggregate parse times across all trace files for the requested headers.

    Returns a nested dictionary of the form::

        {
            "cxb.h": {
                "total_time": float,
                "avg_time": float,
                "max_time": float,
                "std_dev": float,
                "parse_count": int,
            },
            ...
        }
    """
    trace_files = find_trace_files(build_dir)
    print(f"Found {len(trace_files)} trace files")

    durations: Dict[str, List[float]] = defaultdict(list)
    for trace_file in trace_files:
        for header, dur in parse_trace_file(trace_file).items():
            if any(pattern in header for pattern in headers):
                durations[header].append(dur)

    metrics_by_header: Dict[str, Dict[str, float]] = {}
    for header, times in durations.items():
        if not times:
            continue
        total_time = sum(times)
        avg_time = total_time / len(times)
        max_time = max(times)
        if len(times) > 1:
            variance = sum((x - avg_time) ** 2 for x in times) / (len(times) - 1)
            std_dev = math.sqrt(variance)
        else:
            std_dev = 0.0

        metrics_by_header[header] = {
            "total_time": total_time,
            "avg_time": avg_time,
            "max_time": max_time,
            "std_dev": std_dev,
            "parse_count": len(times),
        }

    return metrics_by_header


def get_cpu_info() -> str:
    """Get CPU information for context."""
    try:
        if sys.platform == "darwin":  # macOS
            cmd = ["sysctl", "-n", "machdep.cpu.brand_string"]
            exit_code, stdout, stderr = run_command(cmd)
            if exit_code == 0:
                return stdout.strip()
        elif sys.platform == "linux":
            with open("/proc/cpuinfo", "r") as f:
                for line in f:
                    if "model name" in line:
                        return line.split(":")[1].strip()
        
        return "Unknown CPU"
    except Exception:
        return "Unknown CPU"


def update_readme(readme_path: Path, metrics: Dict[str, Dict[str, float]], headers: List[str]) -> None:
    """Update README.md with compile-time information for each header in *headers*.

    The README is expected to have bullet lines like::

        * `header`: <number>±<std_dev>ms (<count> parses) on <CPU>

    For every header supplied we either replace an existing line (matched via
    a regex) or insert a new one right after the "Current Header Compile
    Times:" section.
    """
    if not readme_path.exists():
        print(f"README.md not found at {readme_path}")
        return

    if not metrics:
        print("No metrics to update")
        return

    with open(readme_path, "r") as f:
        content = f.read()

    cpu_info = get_cpu_info()

    # Track modifications for user feedback.
    lines_to_append: List[str] = []  # newly inserted lines
    lines_updated: List[str] = []    # existing lines that were replaced

    for header in headers:
        # Try to locate the metric entry matching this header (by suffix).
        metric_key = None
        for hdr in metrics:
            if hdr == header or hdr.endswith(f"/{header}") or hdr.endswith(f"\\{header}"):
                metric_key = hdr
                break

        header_display = f"`{header}`"
        if metric_key:
            stats = metrics[metric_key]
            new_line = (
                f"* {header_display}: {stats['avg_time']:.0f}±{stats['std_dev']:.0f}ms "
                f"({stats['parse_count']} parses) on {cpu_info}"
            )
        else:
            new_line = f"* {header_display}: Unable to measure on {cpu_info}"

        pattern = rf"\* {re.escape(header_display)}: .*"
        if re.search(pattern, content):
            # Only log as updated if the replacement actually changes the line.
            original_line_match = re.search(pattern, content)
            original_line = original_line_match.group(0) if original_line_match else ""
            content = re.sub(pattern, new_line, content)
            lines_updated.append(new_line)
        else:
            lines_to_append.append(new_line)

    # If we have new lines to append, insert them after the dedicated section.
    if lines_to_append:
        header_section_pattern = r"(Current Header Compile Times:)"
        if re.search(header_section_pattern, content):
            # Insert after the section header.
            replacement = "\\1\n" + "\n".join(lines_to_append)
            content = re.sub(header_section_pattern, replacement, content, count=1)
        else:
            # If section not found, append at end of file.
            content += "\n" + "\n".join(lines_to_append) + "\n"

    with open(readme_path, "w") as f:
        f.write(content)

    if lines_updated:
        print("Updated lines:")
        for line in lines_updated:
            print(f"  {line}")

    if lines_to_append:
        print("Added lines:")
        for line in lines_to_append:
            print(f"  {line}")


def update_parse_times_md(md_path: Path, metrics: Dict[str, Dict[str, float]], headers: List[str]) -> None:
    """Write a markdown table with parse-time stats for *headers*.

    The table format:

    | Header | Avg (ms) | StdDev (ms) | Parses | CPU |
    |-------|---------|-------------|--------|-----|
    """

    cpu_info = get_cpu_info()

    rows: List[str] = [
        "| Header | Avg (ms) | StdDev (ms) | Parses | CPU |",
        "|--------|---------|-------------|--------|-----|",
    ]

    for header in headers:
        metric_key = None
        for hdr in metrics:
            if hdr.endswith(f"/{header}") or hdr.endswith(f"\\{header}") or hdr == header:
                metric_key = hdr
                break

        if metric_key:
            stats = metrics[metric_key]
            row = (
                f"| `{header}` | {stats['avg_time']:.0f} | {stats['std_dev']:.0f} | "
                f"{stats['parse_count']} | {cpu_info} |")
        else:
            row = f"| `{header}` | - | - | - | {cpu_info} |"

        rows.append(row)

    md_path.parent.mkdir(parents=True, exist_ok=True)
    with open(md_path, "w") as f:
        f.write("\n".join(rows) + "\n")

    print(f"Wrote parse times table to {md_path}")


def main():
    parser = argparse.ArgumentParser(
        description="Analyze build times using Clang's -ftime-trace feature"
    )
    parser.add_argument(
        "--clean", 
        action="store_true", 
        help="Clean the build directory before building"
    )
    parser.add_argument(
        "--headers",
        nargs="+",
        default=["cxb.h"],
        help="Headers to analyze"
    )
    args = parser.parse_args()
    
    script_dir = Path(__file__).parent
    project_dir = script_dir.parent
    build_dir = project_dir / "build"
    readme_path = project_dir / "README.md"
    
    print(f"Project directory: {project_dir}")
    print(f"Build directory: {build_dir}")
    
    exit_code, _, _ = run_command(["clang", "--version"])
    if exit_code != 0:
        sys.stderr.write("Clang not found. This script requires Clang for -ftime-trace support.\n")
        return 2
    
    if args.clean:
        print("Cleaning build directory...")
        clean_build_dir(build_dir)
    elif not build_dir.exists():
        print("Build directory doesn't exist, creating it...")
        build_dir.mkdir(parents=True, exist_ok=True)
    
    cmake_cache = build_dir / "CMakeCache.txt"
    if not cmake_cache.exists() or args.clean:
        print("Configuring CMake...")
        if not configure_cmake(build_dir, project_dir):
            return 3
    else:
        print("Using existing CMake configuration...")
    
    print("Building project...")
    if not build_project(build_dir):
        return 4

    # Combine user-specified headers with the standard-library set to gather all
    # relevant metrics in one pass.
    headers_to_scan = list(set(args.headers + STD_HEADERS))

    print("Analyzing build times...")
    metrics = analyze_parse_times(build_dir, headers_to_scan)
    
    if metrics:
        print("Metrics found:")
        for hdr, stats in metrics.items():
            print(
                f"  {hdr}: avg {stats['avg_time']:.2f}ms "
                f"(±{stats['std_dev']:.2f}ms over {stats['parse_count']} parses)"
            )

        print("Updating README.md...")
        update_readme(readme_path, metrics, args.headers)

        print("Updating parse times markdown...")
        update_parse_times_md(PARSE_TIMES_MD, metrics, STD_HEADERS)
    else:
        print("No metrics found. This might happen if:")
        print("- No trace files were generated")
        print("- No cxb.h parsing events were found")
        print("- Trace files were malformed")
    
    return 0


if __name__ == "__main__":
    sys.exit(main()) 
