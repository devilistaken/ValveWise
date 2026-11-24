import csv
import json
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ENGINE = ROOT / "build" / "valvewise"

class FinalValidation(unittest.TestCase):

    def simulate(self, dataset, failures="-", closures="-"):
        with tempfile.TemporaryDirectory() as tmp:
            output = Path(tmp) / "result.json"

            result = subprocess.run(
                [
                    str(ENGINE),
                    str(ROOT / dataset),
                    failures,
                    str(output),
                    closures
                ],
                capture_output=True,
                text=True,
                timeout=30,
                cwd=ROOT
            )

            self.assertEqual(
                result.returncode,
                0,
                result.stderr
            )

            return json.loads(output.read_text())

    def test_baseline(self):
        data = self.simulate("datasets/test_network.inp")

        self.assertEqual(data["summary"]["total_nodes"], 4)
        self.assertEqual(data["summary"]["connected_nodes"], 4)
        self.assertEqual(data["analysis"]["total_junctions"], 3)
        self.assertEqual(data["analysis"]["source_count"], 1)
        self.assertEqual(data["analysis"]["total_demand"], 60)

    def test_single_failure(self):
        data = self.simulate(
            "datasets/test_network.inp", "P2"
        )

        self.assertEqual(
            data["analysis"]["disconnected_junctions"], 2
        )
        self.assertEqual(
            data["analysis"]["disconnected_demand"], 50
        )

    def test_multiple_failures(self):
        data = self.simulate(
            "datasets/test_network.inp", "P1,P3"
        )

        self.assertEqual(
            data["summary"]["connected_nodes"], 1
        )
        self.assertEqual(
            data["analysis"]["disconnected_demand"], 60
        )

    def test_closure_state(self):
        data = self.simulate(
            "datasets/test_network.inp",
            "-",
            "P2"
        )

        self.assertEqual(
            data["analysis"]["disconnected_demand"], 50
        )

        statuses = {
            link["id"]: link["status"]
            for link in data["links"]
        }

        self.assertEqual(statuses["P2"], "closed")

    def test_failure_and_closure(self):
        data = self.simulate(
            "datasets/test_network.inp",
            "P2",
            "P3"
        )

        self.assertEqual(
            data["analysis"]["disconnected_demand"], 50
        )

    def test_net3_import(self):
        data = self.simulate("datasets/Net3.inp")

        self.assertGreater(
            data["summary"]["total_nodes"], 50
        )
        self.assertGreater(
            data["summary"]["total_links"], 50
        )
        self.assertGreater(
            data["analysis"]["source_count"], 0
        )

    def test_demand_conservation(self):
        data = self.simulate(
            "datasets/test_network.inp", "P2"
        )
        analysis = data["analysis"]

        self.assertAlmostEqual(
            analysis["connected_demand"] +
            analysis["disconnected_demand"],
            analysis["total_demand"]
        )

    def test_junction_conservation(self):
        data = self.simulate(
            "datasets/test_network.inp", "P2"
        )
        analysis = data["analysis"]

        self.assertEqual(
            analysis["connected_junctions"] +
            analysis["disconnected_junctions"],
            analysis["total_junctions"]
        )

    def test_invalid_link_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            result = subprocess.run(
                [
                    str(ENGINE),
                    str(ROOT / "datasets/test_network.inp"),
                    "NONEXISTENT",
                    str(Path(tmp) / "out.json")
                ],
                capture_output=True,
                text=True,
                cwd=ROOT
            )

            self.assertNotEqual(result.returncode, 0)

    def test_original_dataset_is_unchanged(self):
        first = self.simulate(
            "datasets/test_network.inp"
        )

        self.simulate(
            "datasets/test_network.inp",
            "P1,P2"
        )

        second = self.simulate(
            "datasets/test_network.inp"
        )

        self.assertEqual(
            first["summary"],
            second["summary"]
        )

if __name__ == "__main__":
    unittest.main(verbosity=2)
