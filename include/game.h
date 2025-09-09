#ifndef GAME_H_INCLUDED
#define GAME_H_INCLUDED

typedef enum {
    P_NONE = 0,
    P1_PAWN,
    P2_PAWN,
    P1_KING,
    P2_KING
} Piece;

typedef struct {
    int won;
    int turn;
    int selected_tile[2];

    Piece last_visited[9][9];
    Piece board[9][9];
} Game;

Game init_game(void);
void update_board(Game* game, int dst_col, int dst_row);

#endif // GAME_H_INCLUDED
