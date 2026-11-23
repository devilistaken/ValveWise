import json
import subprocess
import tempfile
from http.server import ThreadingHTTPServer, SimpleHTTPRequestHandler
from pathlib import Path
from urllib.parse import urlparse, parse_qs

ROOT = Path(__file__).resolve().parents[2]
FRONTEND = ROOT / "frontend"
ENGINE = ROOT / "build" / "valvewise"

DATASETS = {
    "test": ROOT / "datasets" / "test_network.inp",
    "net3": ROOT / "datasets" / "Net3.inp",
}

MAX_FAILURE_ID_LENGTH = 63


class ValveWiseHandler(SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(
            *args,
            directory=str(FRONTEND),
            **kwargs
        )

    def send_json(self, status, data):
        content = json.dumps(data).encode("utf-8")

        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(content)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(content)

    def do_GET(self):
        parsed = urlparse(self.path)

        if parsed.path == "/api/health":
            return self.send_json(
                200,
                {
                    "status": "ok",
                    "engine": "C",
                    "service": "ValveWise"
                }
            )

        if parsed.path == "/api/network":
            return self.handle_network(parsed.query)

        return super().do_GET()

    def handle_network(self, query):
        params = parse_qs(query)

        dataset_name = params.get("dataset", ["test"])[0]
        failure = params.get("failure", ["-"])[0]

        if dataset_name not in DATASETS:
            return self.send_json(
                400,
                {"error": "Unknown dataset"}
            )

        if (
            len(failure) > MAX_FAILURE_ID_LENGTH
            or not failure
        ):
            return self.send_json(
                400,
                {"error": "Invalid pipe ID"}
            )

        if not ENGINE.is_file():
            return self.send_json(
                503,
                {"error": "C engine not compiled"}
            )

        output_path = None

        try:
            with tempfile.NamedTemporaryFile(
                suffix=".json",
                delete=False
            ) as temp:
                output_path = Path(temp.name)

            cmd = [
                str(ENGINE),
                str(DATASETS[dataset_name]),
                failure,
                str(output_path)
            ]

            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=15,
                cwd=ROOT,
                check=False
            )

            if result.returncode != 0:
                return self.send_json(
                    400,
                    {
                        "error": "Simulation failed",
                        "details": result.stderr.strip()
                    }
                )

            data = json.loads(
                output_path.read_text(encoding="utf-8")
            )

            data["dataset"] = dataset_name
            data["simulated_failure"] = (
                None if failure == "-" else failure
            )

            return self.send_json(200, data)

        except subprocess.TimeoutExpired:
            return self.send_json(
                504,
                {"error": "Simulation timed out"}
            )

        except (OSError, ValueError) as exc:
            return self.send_json(
                500,
                {"error": str(exc)}
            )

        finally:
            if output_path is not None:
                output_path.unlink(missing_ok=True)


if __name__ == "__main__":
    host = "127.0.0.1"
    port = 8080

    server = ThreadingHTTPServer(
        (host, port),
        ValveWiseHandler
    )

    print(f"ValveWise running at http://{host}:{port}")
    print("Press Ctrl+C to stop.")

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()
