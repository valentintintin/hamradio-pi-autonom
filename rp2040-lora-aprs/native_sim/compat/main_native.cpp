// ============================================================================
// Point d'entrée natif — un vrai core Arduino fournit son propre main() qui
// appelle setup() une fois puis loop() en boucle ; ici src/main.cpp ne
// définit que ces deux fonctions (style sketch), donc ce fichier reproduit
// ce runtime minimal pour l'env `native`.
// ============================================================================

#include <cstdio>

extern void setup();
extern void loop();

int main() {
  // stdout est bufferisé par bloc dès qu'il n'est pas un terminal (fichier,
  // pipe...) — ligne par ligne ici pour que les logs/réponses CLI
  // apparaissent immédiatement, y compris redirigés.
  setvbuf(stdout, nullptr, _IOLBF, 0);

  setup();
  for (;;) {
    loop();
  }
  return 0;
}
