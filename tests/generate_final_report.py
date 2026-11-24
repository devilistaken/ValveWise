import csv
import datetime
import json
import platform
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / "build"
REPORT = ROOT / "docs" / "PERSON1_VALIDATION_REPORT.md"

def run(command):
    result = subprocess.run(
        command,
        cwd=ROOT,
        capture_output=True,
        text=True,
        timeout=120
    )

    if result.returncode:
        raise RuntimeError(
            f"Command failed: {' '.join(command)}\n"
            f"{result.stdout}\n{result.stderr}"
        )

    return result.stdout

def main():
    BUILD.mkdir(exist_ok=True)

    benchmark = run([
        str(BUILD / "benchmark_graph")
    ])

    (BUILD / "final_benchmark.csv").write_text(benchmark)

    real_network = BUILD / "net3_failure_report.csv"

    if not real_network.exists():
        raise RuntimeError(
            "Run python3 tests/validate_real_network.py first"
        )

    with real_network.open(newline="") as file:
        rows = list(csv.DictReader(file))

    cases = []
    for failure in ["-", "P2", "P1,P3"]:
        output = BUILD / "report_network.json"

        run([
            str(BUILD / "valvewise"),
            "datasets/test_network.inp",
            failure,
            str(output)
        ])

        data = json.loads(output.read_text())

        cases.append({
            "failure": failure,
            "disconnected_junctions":
                data["analysis"]["disconnected_junctions"],
            "disconnected_demand":
                data["analysis"]["disconnected_demand"]
        })

    lines = [
        "# ValveWise — Person 1 Validation Report",
        "",
        f"Generated: {datetime.datetime.now().isoformat(timespec='seconds')}",
        f"System: {platform.platform()}",
        "",
        "## Scope",
        "",
        "C-based adjacency-list graph engine, hash indexing, "
        "EPANET subset import, BFS/DFS, independent network "
        "states, failure scenarios, demand connectivity and "
        "frontend/API integration.",
        "",
        "## Controlled test network",
        "",
        "| Failed links | Disconnected junctions | Disconnected demand |",
        "|---|---:|---:|"
    ]

    for case in cases:
        lines.append(
            f"| {case['failure']} | "
            f"{case['disconnected_junctions']} | "
            f"{case['disconnected_demand']} |"
        )

    lines += [
        "",
        "## EPANET Net3 sampled failure analysis",
        "",
        f"Scenarios sampled: {len(rows)}",
        "",
        "| Failed pipe | Disconnected junctions | Disconnected demand |",
        "|---|---:|---:|"
    ]

    for row in rows[:10]:
        lines.append(
            f"| {row['failed_pipe']} | "
            f"{row['disconnected_junctions']} | "
            f"{row['disconnected_demand']} |"
        )

    lines += [
        "",
        "## C graph benchmark",
        "",
        "Build time and mean traversal time, in milliseconds.",
        "",
        "```csv",
        benchmark.strip(),
        "```",
        "",
        "## Algorithm complexity",
        "",
        "- BFS: O(V + E) for one traversal.",
        "- DFS: O(V + E) for one traversal.",
        "- Hash lookup: expected O(1), worst case O(H).",
        "- Scenario-state storage: O(MAX_PIPES).",
        "",
        "## Limitations",
        "",
        "- Graph connectivity is not hydraulic flow or pressure.",
        "- Isolation feasibility is not established by BFS alone.",
        "- Source and tank behavior is simplified.",
        "- EPANET input support is partial.",
        "- Existing valves are imported as links, not "
        "necessarily as pipe-end isolation devices.",
        "- The Net3 comparison uses a sampled set of pipes.",
        "- Benchmark results use synthetic chain graphs and "
        "do not measure all possible topologies.",
        "",
        "## Team integration",
        "",
        "Person 2 can use the Graph, NetworkState and "
        "ReachabilityResult interfaces described in "
        "`PERSON2_INTEGRATION.md`.",
        ""
    ]

    REPORT.parent.mkdir(exist_ok=True)
    REPORT.write_text("\n".join(lines))

    print("Created:", REPORT)
    print("Benchmark saved to:", BUILD / "final_benchmark.csv")

if __name__ == "__main__":
    main()
