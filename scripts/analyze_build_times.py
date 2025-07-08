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
    with open(trace_file, 'r') as f:
        data = json.load(f)
    
    header_times = {}
    source_events = {}
    
    events = data.get('traceEvents', [])
    for event in events:
        event_type = event.get('ph')
        name = event.get('name', '')
        timestamp = event.get('ts', 0)
        
        if event_type == 'X':
            duration_us = event.get('dur', 0)
            duration_ms = duration_us / 1000.0
            
            if any(pattern in name for pattern in ['ParseFile', 'ParseClass', 'ParseTemplate', 'InstantiateFunction', 'InstantiateClass']):
                if 'args' in event and 'detail' in event['args']:
                    detail = event['args']['detail']
                    if 'cxb.h' in detail:
                        header_times[detail] = duration_ms
            
            elif 'Source' in name and 'cxb.h' in name:
                header_times[name] = duration_ms
        
        elif event_type == 'b' and name == 'Source':
            if 'args' in event and 'detail' in event['args']:
                detail = event['args']['detail']
                event_id = event.get('id', 0)
                source_events[event_id] = {'start': timestamp, 'detail': detail}
        
        elif event_type == 'e' and name == 'Source':
            event_id = event.get('id', 0)
            if event_id in source_events:
                start_time = source_events[event_id]['start']
                detail = source_events[event_id]['detail']
                duration_us = timestamp - start_time
                duration_ms = duration_us / 1000.0
                header_times[detail] = duration_ms
                del source_events[event_id]
    
    return header_times


def analyze_parse_times(build_dir: Path, headers: List[str]) -> Dict[str, float]:
    trace_files = find_trace_files(build_dir)
    print(f"Found {len(trace_files)} trace files")
    
    metrics_by_header = {}
    header_times = defaultdict(list)
    
    for trace_file in trace_files:
        parsed = parse_trace_file(trace_file)
        for header, time_ms in parsed.items():
            if any(h in header for h in headers):
                header_times[header].append(time_ms)
    
    for header, times in header_times.items():
        total_time = sum(times)
        avg_time = total_time / len(times)
        max_time = max(times)
        
        if len(times) > 1:
            variance = sum((x - avg_time) ** 2 for x in times) / (len(times) - 1)
            std_dev = math.sqrt(variance)
        else:
            std_dev = 0.0
        
        metrics_by_header[header] = {
            'total_time': total_time,
            'avg_time': avg_time,
            'max_time': max_time,
            'std_dev': std_dev,
            'parse_count': len(times)
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


def update_readme(readme_path: Path, metrics: Dict[str, float]) -> None:
    if not readme_path.exists():
        print(f"README.md not found at {readme_path}")
        return
    
    with open(readme_path, 'r') as f:
        content = f.read()
    
    if not metrics:
        print("No metrics to update")
        return
    
    cpu_info = get_cpu_info()
    
    if 'cxb_h_avg_time' in metrics:
        avg_time = metrics['cxb_h_avg_time']
        std_dev = metrics['cxb_h_std_dev']
        parse_count = metrics['cxb_h_parse_count']
        
        new_line = f"* `cxb.h`: {avg_time:.0f}±{std_dev:.0f}ms on {cpu_info}"
    else:
        new_line = f"* `cxb.h`: Unable to measure on {cpu_info}"
    
    pattern = r'\* `cxb\.h`: .*'
    if re.search(pattern, content):
        content = re.sub(pattern, new_line, content)
    else:
        header_pattern = r'(Current Header Compile Times:)\n.*TODO.*'
        if re.search(header_pattern, content):
            content = re.sub(
                header_pattern, 
                f'\\1\n{new_line}', 
                content
            )
    
    with open(readme_path, 'w') as f:
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
    breakpoint()
    
    if metrics:
        print("Metrics found:")
        for key, value in metrics.items():
            print(f"  {key}: {value:.2f}ms")
        
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