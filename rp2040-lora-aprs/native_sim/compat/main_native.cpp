#include <cstdio>

extern void setup();
extern void loop();

int main() {
  // Force stdout en ligne-par-ligne: sinon bufferisé par bloc dès qu'il n'est
  // pas un terminal (fichier, pipe), retardant logs/réponses CLI redirigés.
  setvbuf(stdout, nullptr, _IOLBF, 0);

  setup();
  for (;;) {
    loop();
  }
  return 0;
}
