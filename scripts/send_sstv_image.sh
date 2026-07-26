#!/usr/bin/env bash
# ============================================================================
# send_sstv_image.sh — convertit une image (jpg/png/...) au format attendu par
# le firmware ("image <n>" du CLI série, cf. rp2040-lora-aprs/src/aprs/
# SstvTransmitter.h) et l'uploade, avec option d'enchaîner l'émission.
#
# Format attendu par le firmware : RGB888 brut, sans en-tête, redimensionné
# exactement aux dimensions du mode SSTV configuré à bord (cf. "get
# sstv.mode" sur le CLI) — bornes forcées (ratio non préservé), pas de canal
# alpha. Table des résolutions ci-dessous : DOIT rester cohérente avec
# SSTV_MODE_LIST dans SstvTransmitter.h si de nouveaux modes y sont ajoutés.
#
# Usage: ./send_sstv_image.sh <port> <image> [--mode <nom>] [--send]
#   ex:  ./send_sstv_image.sh /dev/ttyACM0 photo.jpg
#        ./send_sstv_image.sh /dev/ttyACM0 photo.jpg --mode martin1 --send
#
# --mode <nom> : doit correspondre au mode réellement réglé à bord
#                ("set sstv.mode <nom>"), sinon le firmware refusera l'upload
#                (taille annoncée ne correspondant pas au mode courant).
#                Défaut : robot36 (mode par défaut du firmware).
# --send       : enchaîne "image send" après un upload réussi (déclenche la
#                transmission CW+SSTV, ~30s à ~2min selon le mode).
#
# Dépendances : stty, ImageMagick (convert) — pas de Python.
# ============================================================================
set -euo pipefail

# nom -> "largeurxhauteur" — cf. SSTV_MODE_LIST (rp2040-lora-aprs/src/aprs/SstvTransmitter.h)
declare -A SSTV_MODE_DIMS=(
  [robot36]="320x240"
  [robot72]="320x240"
  [martin1]="320x256"
  [martin2]="320x256"
  [scottie1]="320x256"
  [scottie2]="320x256"
  [scottiedx]="320x256"
  [wrasse]="320x256"
)

PORT="${1:?Usage: $0 <port> <image> [--mode <nom>] [--send]}"
IMG="${2:?Usage: $0 <port> <image> [--mode <nom>] [--send]}"
shift 2

MODE="robot36"
DO_SEND=false
while [[ $# -gt 0 ]]; do
  case "$1" in
    --mode)
      MODE="${2:?--mode nécessite un nom}"
      shift 2
      ;;
    --send)
      DO_SEND=true
      shift
      ;;
    *)
      echo "Argument inconnu: $1" >&2
      exit 1
      ;;
  esac
done

if [[ ! -f "$IMG" ]]; then
  echo "Image introuvable: $IMG" >&2
  exit 1
fi

DIMS="${SSTV_MODE_DIMS[$MODE]:-}"
if [[ -z "$DIMS" ]]; then
  echo "Mode inconnu: $MODE (attendu: ${!SSTV_MODE_DIMS[*]})" >&2
  exit 1
fi
W="${DIMS%x*}"
H="${DIMS#*x}"
EXPECTED=$((W * H * 3))

command -v convert >/dev/null || { echo "ImageMagick (convert) requis" >&2; exit 1; }

TMP_RAW="$(mktemp)"
trap 'rm -f "$TMP_RAW"' EXIT

# "!" force les dimensions exactes (pas de préservation du ratio) : le
# firmware exige une taille RGB888 exacte, pas un "au plus proche".
convert "$IMG" -resize "${W}x${H}!" -depth 8 "RGB:$TMP_RAW"

ACTUAL=$(stat -c%s "$TMP_RAW" 2>/dev/null || stat -f%z "$TMP_RAW")
if [[ "$ACTUAL" -ne "$EXPECTED" ]]; then
  echo "Conversion inattendue: $ACTUAL octets (attendu $EXPECTED pour ${W}x${H} RGB888)" >&2
  exit 1
fi

echo "Mode $MODE (${W}x${H}, $EXPECTED octets) -> $PORT"

# Configure la liaison AVANT d'écrire : 115200 8N1, brut (pas d'écho).
stty -F "$PORT" 115200 raw -echo -echoe -echok -ixon -ixoff -crtscts cs8 -parenb -cstopb

# Un seul fd ouvert du début (commande "image <n>") à la fin (dernier octet
# de l'image, et "image send" le cas échéant) : évite toute réouverture du
# port entre l'annonce de taille et les données qui pourrait perdre des
# octets ou perturber la liaison série.
exec 3>"$PORT"
printf 'image %d\n' "$EXPECTED" >&3
sleep 0.2
cat "$TMP_RAW" >&3

if $DO_SEND; then
  sleep 0.5
  printf 'image send\n' >&3
  echo "Transmission demandée (image send)"
else
  echo "Upload envoyé. Pour déclencher l'émission : envoyer 'image send' sur le CLI (série ou APRS)."
fi

exec 3>&-
