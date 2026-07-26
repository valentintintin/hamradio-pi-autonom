#!/usr/bin/env bash
# ============================================================================
# dump_eeprom_csv.sh — récupère "history dump" ou "eventlog dump" via le CLI
# série du firmware (dump binaire EepromDumpHeader + records, cf.
# rp2040-lora-aprs/src/hal/EepromDumpHeader.h) et le convertit en CSV.
#
# Bien plus rapide qu'un "history"/"eventlog" texte sur un historique complet
# (~3x moins de données sur le fil, pas de formatage ASCII des nombres côté
# MCU) : ~10s pour un dump binaire complet de l'historique télémétrie
# (~5400 records max) contre ~30s en CSV direct à 115200 bauds.
#
# Usage: ./dump_eeprom_csv.sh <port> <history|eventlog> [n] > out.csv
#   ex:  ./dump_eeprom_csv.sh /dev/ttyACM0 history         # tout l'historique
#        ./dump_eeprom_csv.sh /dev/ttyACM0 eventlog 50     # 50 derniers événements
#
# Dépendances : stty, python3 (aucune lib externe).
# ============================================================================
set -euo pipefail

PORT="${1:?Usage: $0 <port> <history|eventlog> [n]}"
KIND="${2:?Usage: $0 <port> <history|eventlog> [n]}"
N="${3:-}"

if [[ "$KIND" != "history" && "$KIND" != "eventlog" ]]; then
  echo "Type inconnu: $KIND (attendu: history|eventlog)" >&2
  exit 1
fi

# Configure la liaison AVANT que python n'ouvre le port : 115200 8N1, brut
# (pas d'écho, pas de contrôle de flux logiciel qui pourrait avaler des
# octets 0x11/0x13 présents dans les données binaires).
stty -F "$PORT" 115200 raw -echo -echoe -echok -ixon -ixoff -crtscts cs8 -parenb -cstopb

python3 - "$PORT" "$KIND" "$N" <<'PYEOF'
import struct
import sys
import time

port, kind, n = sys.argv[1], sys.argv[2], sys.argv[3]

# Un seul fd ouvert du début (commande) à la fin (dernier byte du dump) :
# pas de fermeture/réouverture entre l'envoi et la lecture qui risquerait de
# perdre les premiers octets de la réponse.
f = open(port, "r+b", buffering=0)

cmd = f"{kind} dump {n}\n" if n else f"{kind} dump\n"
f.write(cmd.encode())


def read_exact(n_bytes):
    buf = b""
    # Le port série peut rendre les données par petits paquets : on boucle
    # jusqu'à avoir tout reçu plutôt que de supposer un seul read() suffisant.
    deadline = time.monotonic() + 15
    while len(buf) < n_bytes:
        chunk = f.read(n_bytes - len(buf))
        if chunk:
            buf += chunk
        elif time.monotonic() > deadline:
            sys.exit(f"Timeout : {len(buf)}/{n_bytes} octets reçus (port/commande incorrects ?)")
    return buf


header = read_exact(8)
magic, record_size, count = struct.unpack("<IHH", header)

TELH_MAGIC = 0x54454C48  # "TELH" (TelemetryHistory)
EVL2_MAGIC = 0x45564C32  # "EVL2" (EventLogHistory)

if magic == TELH_MAGIC:
    fmt = "<IhhhhhBhI"  # timestamp,bat_mV,bat_mA,sol_mV,sol_mA,temp_in_c10,hum_in,temp_out_c10,uptime_ms
    if record_size != struct.calcsize(fmt):
        sys.exit(f"Taille de record inattendue ({record_size}, attendu {struct.calcsize(fmt)}) "
                  "— firmware/script désynchronisés (TelemetryRecord a changé ?)")
    print("timestamp,bat_mV,bat_mA,sol_mV,sol_mA,temp_in_c,hum_in,temp_out_c,uptime_ms")
    for _ in range(count):
        ts, bat_mv, bat_ma, sol_mv, sol_ma, temp_in, hum_in, temp_out, uptime = struct.unpack(fmt, read_exact(record_size))
        print(f"{ts},{bat_mv},{bat_ma},{sol_mv},{sol_ma},{temp_in / 10:.1f},{hum_in},{temp_out / 10:.1f},{uptime}")

elif magic == EVL2_MAGIC:
    fmt = "<IHii"  # timestamp,code,data0,data1
    if record_size != struct.calcsize(fmt):
        sys.exit(f"Taille de record inattendue ({record_size}, attendu {struct.calcsize(fmt)}) "
                  "— firmware/script désynchronisés (EventLogRecord a changé ?)")
    print("timestamp,code,data0,data1")
    for _ in range(count):
        ts, code, data0, data1 = struct.unpack(fmt, read_exact(record_size))
        print(f"{ts},{code},{data0},{data1}")

else:
    sys.exit(f"Magic inconnu: 0x{magic:08X} (dump vide, port/vitesse incorrects, ou mauvaise commande envoyée ?)")
PYEOF
