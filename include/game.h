#ifndef GAME_H_INCLUDED
#define GAME_H_INCLUDED

typedef enum {
    LOCAL = 0,
    SERVER,
    CLIENT
} GameMode;

typedef enum {
    NOT_PLAYER = 0,
    P1,
    P2
} Player;

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
    int is_ai;

    GameMode game_mode;
    Piece last_visited[9][9];
    Piece board[9][9];
} Game;

Game init_game(GameMode mode, int artificial_intelligence);
int score_player_one(Game game);
int score_player_two(Game game);
void update_board(Game* game, int dst_col, int dst_row);
Player get_player(Piece piece);


#endif // GAME_H_INCLUDED
