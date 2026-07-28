#pragma once

// Sur arduino-pico, Print.h est un header séparé d'Arduino.h — ici la classe
// Print est définie directement dans Arduino.h (cf. ce fichier), donc ce
// header n'existe que pour satisfaire les `#include <Print.h>` du projet
// (ex. core/StringPrint.h) sans les modifier.
#include "Arduino.h"
