import json
import threading
import unittest
from http.server import ThreadingHTTPServer
from urllib.error import HTTPError
from urllib.request import urlopen
from pathlib import Path
import importlib.util

ROOT = Path(__file__).resolve().parents[1]

spec = importlib.util.spec_from_file_location(
    "valvewise_server",
    ROOT / "backend" / "api" / "server.py"
)

module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class TestValveWiseAPI(unittest.TestCase):

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

    def get_json(self, path):
        with urlopen(self.base + path, timeout=10) as response:
            return json.load(response)

    def test_health(self):
        data = self.get_json("/api/health")
        self.assertEqual(data["status"], "ok")
        self.assertEqual(data["engine"], "C")

    def test_normal_network(self):
        data = self.get_json(
            "/api/network?dataset=test"
        )

        self.assertEqual(
            data["summary"]["total_nodes"], 4
        )

        self.assertEqual(
            data["summary"]["connected_nodes"], 4
        )

        self.assertEqual(
            data["summary"]["disconnected_nodes"], 0
        )

    def test_pipe_failure(self):
        data = self.get_json(
            "/api/network?dataset=test&failure=P2"
        )

        self.assertEqual(
            data["summary"]["disconnected_nodes"], 2
        )

        self.assertEqual(
            data["summary"]["disconnected_demand"], 50
        )

        failed = [
            link for link in data["links"]
            if link["id"] == "P2"
        ][0]

        self.assertEqual(failed["status"], "failed")

    def test_reset(self):
        self.get_json(
            "/api/network?dataset=test&failure=P2"
        )

        data = self.get_json(
            "/api/network?dataset=test"
        )

        self.assertEqual(
            data["summary"]["disconnected_nodes"], 0
        )

    def test_invalid_pipe(self):
        with self.assertRaises(HTTPError) as context:
            self.get_json(
                "/api/network?dataset=test&failure=INVALID"
            )

        self.assertEqual(context.exception.code, 400)

    def test_frontend(self):
        with urlopen(self.base + "/", timeout=10) as response:
            html = response.read().decode("utf-8")

        self.assertIn("ValveWise", html)
        self.assertIn("networkSvg", html)


if __name__ == "__main__":
    unittest.main(verbosity=2)
