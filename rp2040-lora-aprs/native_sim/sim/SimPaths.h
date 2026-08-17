#pragma once

#include <string>

std::string simDataDir();

// Boucle manuellement sur chaque segment : POSIX mkdir() ne crée qu'un niveau à la fois.
void simMkdirs(const std::string& dir);
