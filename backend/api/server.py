import re
import json
import subprocess
import tempfile
from http.server import ThreadingHTTPServer, SimpleHTTPRequestHandler
from pathlib import Path
from urllib.parse import urlparse, parse_qs

ROOT = Path(__file__).resolve().parents[2]
FRONTEND = ROOT / "frontend"
ENGINE = ROOT / "build" / "valvewise"
OPTIMIZER_ENGINE = (
    ROOT / "integration/reference/teammate/backend/valvewise"
)
OPTIMIZER_DATASETS = {
    "sample": ROOT / "integration/reference/teammate/data/sample.inp",
    "image3": ROOT / "integration/reference/teammate/data/net_image3.inp",
}

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

        if parsed.path == "/api/optimize":
            return self.handle_optimize(parsed.query)

        if parsed.path == "/api/network":
            return self.handle_network(parsed.query)

        return super().do_GET()

    def handle_optimize(self, query):
        params = parse_qs(query)

        dataset = params.get("dataset", ["sample"])[0]
        failures = params.get("failures", [""])[0]
        stuck = params.get("stuck", [""])[0]
        budget_text = params.get("budget", ["3"])[0]

        if dataset not in OPTIMIZER_DATASETS:
            return self.send_json(400, {"error": "Unknown dataset"})

        if not re.fullmatch(r"[0-9]+", budget_text):
            return self.send_json(400, {"error": "Invalid budget"})

        budget = int(budget_text)
        if budget < 0 or budget > 20:
            return self.send_json(400, {"error": "Budget must be 0-20"})

        valid_ids = re.compile(
            r"^[A-Za-z0-9_.-]+(?:,[A-Za-z0-9_.-]+)*$"
        )

        for value in (failures, stuck):
            if len(value) > 1024 or (
                value and not valid_ids.fullmatch(value)
            ):
                return self.send_json(
                    400, {"error": "Invalid link ID list"}
                )

        if not OPTIMIZER_ENGINE.is_file():
            return self.send_json(
                503, {"error": "Optimizer is not compiled"}
            )

        output_path = None

        try:
            with tempfile.NamedTemporaryFile(
                suffix=".json", delete=False
            ) as temp:
                output_path = Path(temp.name)

            command = [
                str(OPTIMIZER_ENGINE),
                "--input", str(OPTIMIZER_DATASETS[dataset]),
                "--output", str(output_path),
                "--budget", str(budget),
            ]

            if failures:
                command.extend(["--fail", failures])

            if stuck:
                command.extend(["--stuck", stuck])

            result = subprocess.run(
                command,
                capture_output=True,
                text=True,
                cwd=ROOT,
                timeout=30,
                check=False
            )

            if result.returncode != 0:
                return self.send_json(
                    500,
                    {
                        "error": "Optimizer failed",
                        "details": result.stderr[-1500:]
                    }
                )

            data = json.loads(
                output_path.read_text(encoding="utf-8")
            )
            data["dataset"] = dataset
            data["requested_budget"] = budget

            return self.send_json(200, data)

        except subprocess.TimeoutExpired:
            return self.send_json(
                504, {"error": "Optimizer timed out"}
            )

        except (OSError, ValueError) as exc:
            return self.send_json(500, {"error": str(exc)})

        finally:
            if output_path is not None:
                output_path.unlink(missing_ok=True)

    def handle_network(self, query):
        params = parse_qs(query)

        dataset_name = params.get("dataset", ["test"])[0]
        failure = params.get(
            "failures",
            params.get("failure", ["-"])
        )[0]
        closures = params.get("closures", ["-"])[0]

        if dataset_name not in DATASETS:
            return self.send_json(
                400,
                {"error": "Unknown dataset"}
            )

        if (
            not failure
            or not closures
            or len(failure) > 4096
            or len(closures) > 4096
            or any(len(part) > MAX_FAILURE_ID_LENGTH
                   for part in failure.split(","))
            or any(len(part) > MAX_FAILURE_ID_LENGTH
                   for part in closures.split(","))
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
                str(output_path),
                closures
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
            data["simulated_failures"] = (
                [] if failure == "-" else failure.split(",")
            )
            data["simulated_closures"] = (
                [] if closures == "-" else closures.split(",")
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
