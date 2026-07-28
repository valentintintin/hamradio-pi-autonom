#!/usr/bin/env python3
"""Serveur web du simulateur rp2040-lora-aprs (env PlatformIO `native`).

Sert l'interface (native_sim/web/static/) et fait le pont vers le simulateur
C++ (variant_native/, cf. native_sim/web/SimWebBridge.cpp) via deux fichiers
sous SIM_DATA_DIR/web/ :
  - state.json     : instantané d'état, réécrit ~3x/s par le simulateur.
  - commands/*.cmd  : fichiers déposés ici, une ligne de commande CLI par
                      fichier ("sim rx aprs ascii ...", "set relay.2.state
                      on"...) — le simulateur les consomme et les supprime.

Aucune dépendance : bibliothèque standard uniquement (http.server).

Usage :
    python3 native_sim/web/server.py
    SIM_WEB_PORT=9000 SIM_DATA_DIR=/tmp/mon-sim python3 native_sim/web/server.py
"""

import http.server
import json
import os
import time
import uuid
from urllib.parse import unquote_plus

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
STATIC_DIR = os.path.join(SCRIPT_DIR, "static")
PROJECT_ROOT = os.path.abspath(os.path.join(SCRIPT_DIR, "..", ".."))  # rp2040-lora-aprs/

# Racine des données par défaut résolue par rapport à CE script (donc
# indépendante du répertoire courant d'où il est lancé), pas juste
# "native-data" relatif au cwd : évite un mismatch avec le simulateur C++ si
# les deux ne sont pas lancés depuis exactement le même dossier.
DATA_DIR = os.environ.get("SIM_DATA_DIR", os.path.join(PROJECT_ROOT, "native-data"))
WEB_DIR = os.path.join(DATA_DIR, "web")
COMMANDS_DIR = os.path.join(WEB_DIR, "commands")
STATE_FILE = os.path.join(WEB_DIR, "state.json")

PORT = int(os.environ.get("SIM_WEB_PORT", "8088"))

# Le simulateur crée déjà ces dossiers au boot, mais on peut être lancé avant
# lui (ou avec un SIM_DATA_DIR différent par erreur) — les créer nous-mêmes
# évite un crash bête sur la première requête.
os.makedirs(COMMANDS_DIR, exist_ok=True)


def submit_command(cmd, timeout=1.0):
    """Dépose une commande CLI et attend (jusqu'à `timeout`s) qu'elle soit
    consommée par le simulateur, pour que le prochain GET /api/state la
    reflète déjà — écriture atomique (tmp + rename) : le simulateur ne doit
    jamais lire un fichier de commande à moitié écrit."""
    name = f"{time.time_ns()}_{uuid.uuid4().hex[:6]}.cmd"
    path = os.path.join(COMMANDS_DIR, name)
    tmp = path + ".tmp"
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
        pass  # le simulateur a déjà ses propres logs ; pas besoin de dupliquer ici

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
        # Fichiers statiques (index.html/style.css/app.js) : gérés par
        # SimpleHTTPRequestHandler, qui sait déjà faire ça correctement
        # (types MIME, 404 propres, etc.) — pas besoin de le réinventer.
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
            cmd = f"sim rx {which} " + (f"ascii {payload}" if fmt == "ascii" else payload)
            submit_command(cmd)
            self._send_json({"ok": True})

        elif self.path == "/api/relay":
            n = params.get("n", "0")
            on = params.get("on", "0") == "1"
            submit_command(f"set relay.{n}.state {'on' if on else 'off'}")
            self._send_json({"ok": True})

        elif self.path == "/api/cmd":
            # Canal générique : dépose n'importe quelle ligne de commande CLI
            # telle quelle (clockdate, save, beacon, wx, send aprs, set
            # <clé> <valeur>, sim set mppt/victron ...). Pas d'allowlist : ce
            # serveur n'écoute qu'en local, pour piloter un simulateur de
            # test, pas un service exposé.
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
