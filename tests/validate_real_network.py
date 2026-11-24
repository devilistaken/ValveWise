import csv
import json
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ENGINE = ROOT / "build" / "valvewise"
DATASET = ROOT / "datasets" / "Net3.inp"

REPORT = ROOT / "build" / "net3_failure_report.csv"

def simulate(failure="-"):
    with tempfile.TemporaryDirectory() as directory:
        output = Path(directory) / "result.json"

        command = [
            str(ENGINE),
            str(DATASET),
            failure,
            str(output)
        ]

        result = subprocess.run(
            command,
            cwd=ROOT,
            capture_output=True,
            text=True,
            timeout=30
        )

        if result.returncode != 0:
            raise RuntimeError(
                f"Simulation failed for {failure}:\n"
                f"{result.stderr}"
            )

        return json.loads(output.read_text())

def main():
    baseline = simulate()

    nodes = baseline["nodes"]
    links = baseline["links"]

    print("\n===== REAL EPANET NETWORK =====")
    print("Nodes:", len(nodes))
    print("Links:", len(links))
    print("Sources:", baseline["analysis"]["source_count"])
    print(
        "Demand junctions:",
        baseline["analysis"]["total_junctions"]
    )

    failures = [
        link["id"]
        for link in links
        if link["type"] == "pipe"
    ][:30]

    rows = []

    for pipe_id in failures:
        result = simulate(pipe_id)
        analysis = result["analysis"]

        rows.append({
            "failed_pipe": pipe_id,
            "disconnected_junctions":
                analysis["disconnected_junctions"],
            "disconnected_demand":
                analysis["disconnected_demand"],
            "demand_connectivity_percent":
                analysis["demand_connectivity_percent"]
        })

    rows.sort(
        key=lambda row: row["disconnected_demand"],
        reverse=True
    )

    REPORT.parent.mkdir(exist_ok=True)

    with REPORT.open("w", newline="") as file:
        writer = csv.DictWriter(
            file,
            fieldnames=[
                "failed_pipe",
                "disconnected_junctions",
                "disconnected_demand",
                "demand_connectivity_percent"
            ]
        )

        writer.writeheader()
        writer.writerows(rows)

    print("\n===== TOP SAMPLED FAILURES =====")

    for row in rows[:10]:
        print(
            f"{row['failed_pipe']:12s} "
            f"Disconnected junctions: "
            f"{row['disconnected_junctions']:4d} | "
            f"Demand lost: "
            f"{row['disconnected_demand']:.2f}"
        )

    print("\nScenarios evaluated:", len(rows))
    print("CSV report:", REPORT)

if __name__ == "__main__":
    main()
