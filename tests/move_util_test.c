#include "move_util_test.h"
#include "game.h"
#include <stdio.h>
#include <stdlib.h>

/* Mappage des colonnes A-I */
const char COLS_MAP[10] = "ABCDEFGHI";

int col_from_letter(char L) {
    for (int i = 0; i < 9; i++) if (COLS_MAP[i] == L) return i;
    return -1;
}

#ifdef TEST_BUILD
// Versions simplifiées pour les tests (sans GTK)
typedef struct {
    Game *game;
    int sr, sc, dr, dc;
} MoveTask;

int apply_move_idle(void *data) {
    MoveTask *t = (MoveTask*)data;
    /* Version simplifiée pour tests */
    t->game->selected_tile[0] = t->sr;
    t->game->selected_tile[1] = t->sc;

    // Simuler update_board simplement
    if (is_move_legal(t->game, t->sr, t->sc, t->dr, t->dc)) {
        Piece piece = t->game->board[t->sr][t->sc];
        t->game->board[t->sr][t->sc] = P_NONE;
        t->game->board[t->dr][t->dc] = piece;
        t->game->turn++;
    }

    free(t);
    return 0; // G_SOURCE_REMOVE équivalent
}

void post_move_to_gtk(Game *game, const char m[4]) {
    int sc = col_from_letter(m[0]);
    int sr = 9 - (m[1] - '0');
    int dc = col_from_letter(m[2]);
    int dr = 9 - (m[3] - '0');

    if (sc < 0 || dc < 0 || sr < 0 || sr > 8 || dr < 0 || dr > 8) {
        printf("[TEST] Mouvement invalide: %c%c%c%c\n", m[0],m[1],m[2],m[3]);
        return;
    }

    MoveTask *t = malloc(sizeof(MoveTask));
    if (!t) return;

    t->game = game;
    t->sr = sr;
    t->sc = sc;
    t->dr = dr;
    t->dc = dc;

    apply_move_idle(t);
}
#endif
