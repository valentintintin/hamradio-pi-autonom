#pragma once

// ============================================================================
// Version firmware — source unique, partagée entre MyMesh (rôle MeshCore) et
// CommandHandler (commande "version")
// ============================================================================

#ifndef FIRMWARE_VERSION
  #define FIRMWARE_VERSION "v0.1.0"
#endif
#ifndef FIRMWARE_BUILD_DATE
  #define FIRMWARE_BUILD_DATE __DATE__
#endif
