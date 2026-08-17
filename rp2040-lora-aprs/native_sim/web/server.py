#!/usr/bin/env python3
import http.server
import json
import os
import time
import uuid
from urllib.parse import unquote_plus

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
STATIC_DIR = os.path.join(SCRIPT_DIR, "static")
PROJECT_ROOT = os.path.abspath(os.path.join(SCRIPT_DIR, "..", ".."))

# Résolu par rapport à ce script, pas au cwd : évite un mismatch de dossier
# avec le simulateur C++ si les deux ne sont pas lancés depuis le même endroit.
DATA_DIR = os.environ.get("SIM_DATA_DIR", os.path.join(PROJECT_ROOT, "native-data"))
WEB_DIR = os.path.join(DATA_DIR, "web")
COMMANDS_DIR = os.path.join(WEB_DIR, "commands")
STATE_FILE = os.path.join(WEB_DIR, "state.json")

PORT = int(os.environ.get("SIM_WEB_PORT", "8088"))

os.makedirs(COMMANDS_DIR, exist_ok=True)


def submit_command(cmd, timeout=1.0):
    name = f"{time.time_ns()}_{uuid.uuid4().hex[:6]}.cmd"
    path = os.path.join(COMMANDS_DIR, name)
    tmp = path + ".tmp"  # écriture atomique : le simulateur ne doit jamais lire un fichier à moitié écrit
    with open(tmp, "w") as f:
        f.write(cmd)
    os.rename(tmp, path)

    deadline = time.time() + timeout
    while os.path.exists(path) and time.time() < deadline:
        time.sleep(0.02)


def parse_form(body: str):
    params = {}
    for pair in body.split("&"):
        if "=" not in pair:
            continue
        k, v = pair.split("=", 1)
        params[unquote_plus(k)] = unquote_plus(v)
    return params


class Handler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=STATIC_DIR, **kwargs)

    def log_message(self, fmt, *args):
        pass

    def _send_json(self, obj, code=200):
        body = json.dumps(obj).encode()
        self.send_response(code)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        if self.path == "/api/state":
            try:
                with open(STATE_FILE, "rb") as f:
                    data = f.read()
            except FileNotFoundError:
                self._send_json({"error": "simulateur pas encore démarré (ou SIM_DATA_DIR différent)"}, 503)
                return
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(data)))
            self.end_headers()
            self.wfile.write(data)
            return
        super().do_GET()

    def do_POST(self):
        length = int(self.headers.get("Content-Length", 0))
        body = self.rfile.read(length).decode("utf-8", errors="replace")
        params = parse_form(body)

        if self.path == "/api/sensor":
            if "battery_mv" in params:
                cmd = f"sim set battery {params['battery_mv']}"
                if "battery_ma" in params:
                    cmd += f" {params['battery_ma']}"
                submit_command(cmd)
            if "solar_mv" in params:
                cmd = f"sim set solar {params['solar_mv']}"
                if "solar_ma" in params:
                    cmd += f" {params['solar_ma']}"
                submit_command(cmd)
            if "board5v_mv" in params:
                cmd = f"sim set board5v {params['board5v_mv']}"
                if "board5v_ma" in params:
                    cmd += f" {params['board5v_ma']}"
                submit_command(cmd)
            if all(k in params for k in ("temp_c", "humidity", "pressure_hpa")):
                submit_command(f"sim set weather {params['temp_c']} {params['humidity']} {params['pressure_hpa']}")
            self._send_json({"ok": True})

        elif self.path == "/api/rx":
            which = params.get("which", "aprs")
            fmt = params.get("format", "hex")
            payload = params.get("payload", "")
            opts = ""
            if params.get("rssi", "").strip():
                opts += f"rssi {params['rssi']} "
            if params.get("snr", "").strip():
                opts += f"snr {params['snr']} "
            cmd = f"sim rx {which} {opts}" + (f"ascii {payload}" if fmt == "ascii" else payload)
            submit_command(cmd)
            self._send_json({"ok": True})

        elif self.path == "/api/relay":
            n = params.get("n", "0")
            on = params.get("on", "0") == "1"
            submit_command(f"relay {n} {'on' if on else 'off'}")
            self._send_json({"ok": True})

        elif self.path == "/api/cmd":
            # Pas d'allowlist volontairement : serveur local uniquement, pour piloter
            # un simulateur de test — jamais destiné à être exposé sur le réseau.
            cmd = params.get("cmd", "").strip()
            if not cmd:
                self._send_json({"error": "cmd manquant"}, 400)
                return
            submit_command(cmd)
            self._send_json({"ok": True})

        else:
            self._send_json({"error": "not found"}, 404)


def main():
    httpd = http.server.ThreadingHTTPServer(("0.0.0.0", PORT), Handler)
    print(f"Dashboard sur http://localhost:{PORT}  (données : {os.path.abspath(DATA_DIR)})")
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        pass


if __name__ == "__main__":
    main()
