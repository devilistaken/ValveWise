import importlib.util
import json
import threading
import unittest
from http.server import ThreadingHTTPServer
from pathlib import Path
from urllib.parse import urlencode
from urllib.request import urlopen
from urllib.error import HTTPError

ROOT = Path(__file__).resolve().parents[1]

spec = importlib.util.spec_from_file_location(
    "valvewise_api", ROOT / "backend/api/server.py"
)
api = importlib.util.module_from_spec(spec)
spec.loader.exec_module(api)


class CompleteIntegrationTests(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.server = ThreadingHTTPServer(
            ("127.0.0.1", 0), api.ValveWiseHandler
        )
        cls.thread = threading.Thread(
            target=cls.server.serve_forever,
            daemon=True
        )
        cls.thread.start()
        cls.base = "http://127.0.0.1:" + str(
            cls.server.server_port
        )

    @classmethod
    def tearDownClass(cls):
        cls.server.shutdown()
        cls.server.server_close()
        cls.thread.join()

    def analyze(self, failure="P2", budget=1):
        query = urlencode({
            "dataset": "demo",
            "failures": failure,
            "budget": budget
        })
        with urlopen(
            self.base + "/api/optimize?" + query,
            timeout=40
        ) as response:
            return json.load(response)

    def test_homepage(self):
        with urlopen(self.base + "/") as response:
            html = response.read().decode()
        self.assertIn("graphSvg", html)
        self.assertIn("/js/unified.js", html)

    def test_budget_improvement(self):
        zero = self.analyze(budget=0)
        one = self.analyze(budget=1)
        two = self.analyze(budget=2)

        a = zero["optimization"]["greedy"]
        b = one["optimization"]["greedy"]
        c = two["optimization"]["greedy"]

        self.assertEqual(a["placed_count"], 0)
        self.assertLess(
            b["total_disruption"],
            b["baseline_disruption"]
        )
        self.assertLessEqual(
            c["total_disruption"],
            b["total_disruption"] + 1e-8
        )

    def test_recommendations_are_valid(self):
        data = self.analyze()
        ids = {
            link["id"]
            for link in data["graph"]["links"]
        }
        selected = data["optimization"]["greedy"]["placed_valves"]
        self.assertTrue(set(selected).issubset(ids))

    def test_different_failure(self):
        a = self.analyze("P2")
        b = self.analyze("P5")
        self.assertNotEqual(
            a["isolation"]["failed_links"],
            b["isolation"]["failed_links"]
        )

    def test_invalid_pipe_rejected(self):
        query = urlencode({
            "dataset": "demo",
            "failures": "INVALID_PIPE",
            "budget": 1
        })
        with self.assertRaises(HTTPError) as context:
            urlopen(self.base + "/api/optimize?" + query)
        self.assertEqual(context.exception.code, 400)
        context.exception.close()

    def test_algorithm_results(self):
        data = self.analyze()
        optimization = data["optimization"]

        for name in [
            "greedy", "random", "topology", "exhaustive"
        ]:
            self.assertIsNotNone(optimization.get(name))


if __name__ == "__main__":
    unittest.main(verbosity=2)
