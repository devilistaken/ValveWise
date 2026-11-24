import json
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BIN = ROOT / "integration/reference/teammate/backend/valvewise"
DATA = ROOT / "integration/reference/teammate/data/resilience_demo.inp"

def run(failure, budget):
    with tempfile.TemporaryDirectory() as directory:
        output = Path(directory) / "result.json"

        result = subprocess.run(
            [
                str(BIN),
                "--input", str(DATA),
                "--fail", failure,
                "--budget", str(budget),
                "--output", str(output)
            ],
            capture_output=True,
            text=True,
            timeout=30
        )

        if result.returncode != 0:
            raise RuntimeError(result.stderr)

        return json.loads(output.read_text())

baseline = run("P2", 0)
optimized = run("P2", 1)
larger_budget = run("P2", 2)
different_failure = run("P5", 1)

b0 = baseline["optimization"]["greedy"]
b1 = optimized["optimization"]["greedy"]
b2 = larger_budget["optimization"]["greedy"]
other = different_failure["optimization"]["greedy"]

print("\n===== VERIFIED OPTIMIZATION RESULTS =====")

for name, result in [
    ("P2, budget 0", b0),
    ("P2, budget 1", b1),
    ("P2, budget 2", b2),
    ("P5, budget 1", other)
]:
    print(
        f"{name}: "
        f"baseline={result['baseline_disruption']}, "
        f"optimized={result['total_disruption']}, "
        f"improvement={result['improvement_pct']}%, "
        f"valves={result['placed_valves']}"
    )

assert b0["placed_count"] == 0
assert b0["improvement_pct"] == 0

assert b1["total_disruption"] < b1["baseline_disruption"], \
    "Optimizer still shows no real improvement"

assert b1["improvement_pct"] > 0, \
    "Expected positive improvement"

assert b1["placed_count"] == 1, \
    "Budget 1 should select exactly one beneficial valve"

assert len(b1["placed_valves"]) == 1, \
    "Recommended valve list does not match placed count"

assert b1["placed_valves"][0] in {
    link["id"] for link in optimized["graph"]["links"]
}, "Recommended valve does not exist in the graph"

assert b2["total_disruption"] <= b1["total_disruption"], \
    "Increasing budget unexpectedly worsened the greedy solution"

print("\nPASS: Valve placement produces a measurable improvement.")
print("PASS: Budget 0 installs no valves.")
print("PASS: Increasing budget does not worsen this scenario.")
