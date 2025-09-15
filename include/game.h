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
    P2,
    // AI, // AI player or a better check !, use rules (client is blue)
    DRAW // Or we can make a Winner enum ?
} Player;

typedef enum {
    P_NONE = 0,
    P1_PAWN,
    P2_PAWN,
    P1_KING,
    P2_KING,
    P1_VISITED,
    P2_VISITED
} Piece;

typedef enum {
    DIR_TOP,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT,
    NONE
} Direction;

typedef struct {
    int won;
    int turn;
    int selected_tile[2];
    int is_ai;

    GameMode game_mode;
    Piece board[9][9];
} Game;

// Game rules checkers
void won(Game* game);
void did_eat(Game* game, int row, int col, Direction sprint_direction);
int is_move_legal(Game* game, int src_row, int src_col, int dst_row, int dst_col);
Player get_player(Piece piece);

// Game stats
int score_player_one(Game game);
int score_player_two(Game game);

// API used in server, client, AI, input
void update_board(Game* game, int dst_row, int dst_col);
Game init_game(GameMode mode, int artificial_intelligence);


#endif // GAME_H_INCLUDED
