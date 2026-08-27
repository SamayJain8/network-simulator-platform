from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json
import os


analysis_cache = {}


def prediction_for(sample):
    latency = float(sample["latency_ns"])
    drops = int(sample["dropped_packets"])
    throughput = float(sample["throughput_kbps"])
    is_anomaly = latency > 300 or drops > 0
    probability = min(
        1.0,
        (0.4 if latency > 300 else 0.0)
        + (0.3 if drops > 0 else 0.0)
        + (0.2 if throughput < 700 else 0.0),
    )
    link = f"{sample['source_node']}->{sample['dest_node']}"

    return {
        "is_anomaly": is_anomaly,
        "anomaly_probability": probability,
        "predicted_bottleneck_node": sample["dest_node"] if is_anomaly else None,
        "alert_message": (
            f"Predicted latency anomaly ({latency:.1f} ns) on link {link} within the next 3 intervals."
            if is_anomaly
            else "Link stability within normal operating parameters."
        ),
        "recommended_action": (
            f"Congestion imminent. Set Link Cost for {link} to 999999 to trigger dynamic Dijkstra routing bypass."
            if is_anomaly
            else "Maintain current routing table configuration."
        ),
    }


class Handler(BaseHTTPRequestHandler):
    def send_json(self, status, payload):
        encoded = json.dumps(payload).encode("utf-8")
        self.send_response(status)
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(encoded)))
        self.end_headers()
        self.wfile.write(encoded)

    def do_GET(self):
        if self.path == "/health":
            self.send_json(200, {"status": "healthy", "service": "portfolio-telemetry-receiver"})
            return

        if self.path == "/telemetry/analysis":
            self.send_json(200, analysis_cache)
            return

        self.send_json(404, {"error": "not found"})

    def do_POST(self):
        if self.path != "/telemetry/stream":
            self.send_json(404, {"error": "not found"})
            return

        length = int(self.headers.get("Content-Length", "0"))
        sample = json.loads(self.rfile.read(length).decode("utf-8"))
        link = f"{sample['source_node']}->{sample['dest_node']}"
        prediction = prediction_for(sample)
        analysis_cache[link] = prediction
        print(
            f"[portfolio] ingested {link} latency={sample['latency_ns']} "
            f"drops={sample['dropped_packets']} anomaly={prediction['is_anomaly']}",
            flush=True,
        )
        self.send_json(201, prediction)


if __name__ == "__main__":
    host = os.environ.get("HOST", "127.0.0.1")
    port = int(os.environ.get("PORT", "8000"))
    print(f"[portfolio] telemetry receiver listening on http://{host}:{port}", flush=True)
    ThreadingHTTPServer((host, port), Handler).serve_forever()
