#ifndef MOVE_UTIL_TEST_H
#define MOVE_UTIL_TEST_H

#include "game.h"

/* Mappage des colonnes A-I */
extern const char COLS_MAP[10];

/* Convertit une lettre de colonne en indice (0..8) */
int col_from_letter(char L);

#ifdef TEST_BUILD
/* Versions simplifiées pour les tests (sans GTK) */
int apply_move_idle(void *data);
void post_move_to_gtk(Game *game, const char m[4]);
#endif

#endif /* MOVE_UTIL_TEST_H */
