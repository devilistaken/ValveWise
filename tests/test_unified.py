import importlib.util
import json
import threading
import unittest
from http.server import ThreadingHTTPServer
from pathlib import Path
from urllib.request import urlopen

ROOT = Path(__file__).resolve().parents[1]

spec = importlib.util.spec_from_file_location(
    "valvewise_api",
    ROOT / "backend/api/server.py"
)

module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class UnifiedTests(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.server = ThreadingHTTPServer(
            ("127.0.0.1", 0),
            module.ValveWiseHandler
        )
        cls.thread = threading.Thread(
            target=cls.server.serve_forever,
            daemon=True
        )
        cls.thread.start()
        cls.base = (
            f"http://127.0.0.1:{cls.server.server_port}"
        )

    @classmethod
    def tearDownClass(cls):
        cls.server.shutdown()
        cls.server.server_close()
        cls.thread.join()

    def request(self, endpoint):
        with urlopen(self.base + endpoint, timeout=35) as response:
            self.assertEqual(response.status, 200)
            return json.load(response)

    def test_person1_connectivity(self):
        data = self.request(
            "/api/network?dataset=test&failures=P2"
        )
        self.assertEqual(
            data["summary"]["disconnected_demand"], 50
        )

    def test_person2_optimization(self):
        data = self.request(
            "/api/optimize?dataset=sample&failures=P5&budget=3"
        )

        for key in [
            "graph", "reachability", "isolation",
            "structural", "optimization"
        ]:
            self.assertIn(key, data)

        self.assertIn("greedy", data["optimization"])

    def test_frontend_integration(self):
        with urlopen(self.base + "/", timeout=10) as response:
            html = response.read().decode()

        self.assertIn("/js/optimizer.js", html)


if __name__ == "__main__":
    unittest.main(verbosity=2)
