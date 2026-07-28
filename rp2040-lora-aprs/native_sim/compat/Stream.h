#pragma once

// Sur arduino-pico, Stream.h est un header séparé d'Arduino.h — ici la
// classe Stream est définie directement dans Arduino.h (cf. ce fichier),
// donc ce header n'existe que pour satisfaire les `#include <Stream.h>` du
// projet/MeshCore sans les modifier.
#include "Arduino.h"
