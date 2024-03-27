#!/bin/bash

# Vérifier si le fichier de verrouillage existe
if pgrep -f "serial_message" >/dev/null; then
  echo "Script is already running. Exiting."
  exit 1
fi

# Configuration
SERIAL_PORT="/dev/ttyUSB0"  # Port série à écouter
API_ENDPOINT="http://example.com/api"  # Endpoint de l'API

# Écouter le port série en continu
socat -d -d pty,raw,echo=0,link=/tmp/virtualcom0,ignoreeof,b38400 "$SERIAL_PORT" &

# Boucle pour écouter le port série
while true; do
    # Lire une ligne du port série
    json=$(head -n 1 < /tmp/virtualcom0)

    # Vérifier si le JSON est différent du précédent et n'est pas vide
    if [ -n "$json" ]; then
        # Envoyer le JSON à l'API via une requête POST
        curl -X POST -H "Content-Type: application/json" -d "$json" "$API_ENDPOINT"
    fi

    # Attendre un court instant avant de lire la prochaine ligne
    sleep 1
done
