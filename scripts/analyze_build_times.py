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

    # Handle single-event (\'X\') entries first.
    for ev in data.get("traceEvents", []):
        if ev.get("ph") != "X":
            continue
        if ev.get("name") != "Source":  # We only want header parse events.
            continue
        args = ev.get("args", {})
        header_path = args.get("file") or args.get("detail")
        if not header_path:
            continue
        dur_ms = ev.get("dur", 0) / 1000.0  # microseconds → ms
        header_times[header_path] = dur_ms

    # Handle begin/end pair events. Use a stack per thread (tid) because nested
    # Source events occur frequently when headers include other headers.
    open_events: Dict[int, List[Tuple[str, int]]] = defaultdict(list)  # tid → stack[(header, start_ts)]
    for ev in data.get("traceEvents", []):
        name = ev.get("name")
        ph = ev.get("ph")
        if name != "Source" or ph not in {"B", "E"}:
            continue
        tid = ev.get("tid", 0)  # Thread id, guarantees correct pairing.
        if ph == "B":
            args = ev.get("args", {})
            header_path = args.get("file") or args.get("detail")
            if not header_path:
                continue
            open_events[tid].append((header_path, ev.get("ts", 0)))
        elif ph == "E" and open_events[tid]:
            header_path, start_ts = open_events[tid].pop()
            dur_ms = (ev.get("ts", 0) - start_ts) / 1000.0
            # Prefer the longer duration if we already have an entry from the
            # single-event pass – this helps deduplicate nested parse records.
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


def update_readme(readme_path: Path, metrics: Dict[str, Dict[str, float]]) -> None:
    """Update the README with the latest metrics.

    Expects *metrics* in the nested format returned by ``analyze_parse_times``.
    Only the header ``cxb.h`` is currently written to the README, but the
    function can be extended easily for additional headers.
    """
    if not readme_path.exists():
        print(f"README.md not found at {readme_path}")
        return

    if not metrics:
        print("No metrics to update")
        return

    target_header = None
    for hdr in metrics:
        if hdr == "cxb.h" or hdr.endswith("/cxb.h") or hdr.endswith("\\cxb.h"):
            target_header = hdr
            break

    with open(readme_path, "r") as f:
        content = f.read()

    cpu_info = get_cpu_info()

    if target_header is not None:
        stats = metrics[target_header]
        avg_time = stats["avg_time"]
        std_dev = stats["std_dev"]
        parse_count = stats["parse_count"]
        new_line = (
            f"* `cxb.h`: {avg_time:.0f}±{std_dev:.0f}ms "
            f"({parse_count} parses) on {cpu_info}"
        )
    else:
        new_line = f"* `cxb.h`: Unable to measure on {cpu_info}"

    pattern = r"\* `cxb\.h`: .*"
    if re.search(pattern, content):
        content = re.sub(pattern, new_line, content)
    else:
        header_pattern = r"(Current Header Compile Times:)\n.*TODO.*"
        if re.search(header_pattern, content):
            content = re.sub(header_pattern, f"\\1\n{new_line}", content)

    with open(readme_path, "w") as f:
        f.write(content)

    print(f"Updated README.md with metrics: {new_line}")


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
    
    print("Analyzing build times...")
    metrics = analyze_parse_times(build_dir, args.headers)
    
    if metrics:
        print("Metrics found:")
        for hdr, stats in metrics.items():
            print(
                f"  {hdr}: avg {stats['avg_time']:.2f}ms "
                f"(±{stats['std_dev']:.2f}ms over {stats['parse_count']} parses)"
            )

        print("Updating README.md...")
        update_readme(readme_path, metrics)
    else:
        print("No metrics found. This might happen if:")
        print("- No trace files were generated")
        print("- No cxb.h parsing events were found")
        print("- Trace files were malformed")
    
    return 0


if __name__ == "__main__":
    sys.exit(main()) 
