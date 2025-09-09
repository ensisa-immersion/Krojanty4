#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define N 9
#define MAX_CAP 16

/**
 * Structure représentant un déplacement utilisateur.
 * Cela permet de savoir dans quel direction, de combien de cases, un pion va être déplacé.
 */
struct user_move_s
{
    int delta_x; // changement en x (ligne)
    int delta_y; // changement en y (colonne)
};
/**
 * Alias pour struct user_move_s.
 */
typedef struct user_move_s user_move;

/**
 * Valide si les coordonnées sont dans le format attendu.
 * @param col Colonne (A-I).
 * @param row Ligne (1-9).
 * @return 1 si valide, 0 sinon.
 */
int coord_valide(char col, char row)
{
    return (col >= 'A' && col <= 'I' && row >= '1' && row <= '9');
}

/**
 * Valide l'entrée du joueur.
 * @param buffer Chaîne de caractères contenant l'entrée.
 * @return 1 si l'entrée est valide, 0 sinon.
 */
int entree_valide(const char *buffer)
{
    // Format attendu: "A2:A3"
    if (strlen(buffer) != 5 || buffer[2] != ':')
        return 0;
    return coord_valide(buffer[0], buffer[1]) && coord_valide(buffer[3], buffer[4]);
}

/**
 * Parse l'entrée du joueur en un déplacement.
 * @param buffer Chaîne de caractères contenant l'entrée.
 * @return Structure user_move représentant le déplacement.
 */
user_move parse_move(const char *buffer)
{
    user_move move;
    move.delta_x = (buffer[3] - 'A') - (buffer[0] - 'A'); // colonne
    move.delta_y = (buffer[4] - '1') - (buffer[1] - '1'); // ligne
    return move;
}

/**
 * Vérifie si le déplacement est diagonal (interdit).
 * @param buffer Chaîne de caractères contenant l'entrée.
 * @return 1 si le déplacement n'est pas diagonal, 0 sinon.
 */
int deplacement_diagonale_interdit(user_move move)
{
    return !(abs(move.delta_x) == abs(move.delta_y) && abs(move.delta_x) != 0);
}

// compute_captures.c
// Détecte les captures résultant d'un déplacement orthogonal sur un plateau 9x9.
// Retourne le nombre de pions capturés et remplit captured[i][0]=x, captured[i][1]=y.
//
// Représentation attendue du board : board[row][col] with 0..8 indices
// - '.' pour case vide
// - 'b' pour pions joueur A (ou 'B' pour roi par exemple) ; 'r' pour adversaire (adaptable)
// Tu peux appeler compute_captures avec les caractères appropriés pour player/opponent.

// static inline bool in_board(int x, int y) { return x >= 0 && x < N && y >= 0 && y < N; }

/*
  Politique implémentée :
  - Linca (sandwich) : si après le mouvement une case adjacente contient un ennemi
    et que la case juste derrière cet ennemi (même direction) contient une pièce alliée,
    alors l'ennemi est capturé.
  - Seultou (prise en poussant) — interprétation effectuée ici :
    si après le mouvement une case adjacente contient un ennemi et que la case derrière
    cet ennemi (même direction) est vide ('.'), alors l'ennemi est aussi capturé.
    -> Cette interprétation suit "prise en poussant : si personne en soutien derrière".
    Si tu veux plutôt "capturer seulement si l'ennemi est poussé contre un mur/cité"
    ou d'autres variantes, je peux modifier la règle facilement.
*/

/**
 *- Linca (sandwich) : si après le mouvement une case adjacente contient un ennemi
 *  et que la case juste derrière cet ennemi (même direction) contient une pièce alliée,
 *  alors l'ennemi est capturé.
 *
 *- Seultou (prise en poussant) — interprétation effectuée ici :
 *  si après le mouvement une case adjacente contient un ennemi et que la case derrière
 *  cet ennemi (même direction) est vide ('.'), alors l'ennemi est aussi capturé.
 *  -> Cette interprétation suit "prise en poussant : si personne en soutien derrière".
 *  Si tu veux plutôt "capturer seulement si l'ennemi est poussé contre un mur/cité"
 *  ou d'autres variantes, je peux modifier la règle facilement.
 *
 * @param board Plateau de jeu 9x9.
 * @param sx Coordonnée source x (ligne).
 * @param sy Coordonnée source y (colonne).
 * @param dx Coordonnée destination x (ligne).
 * @param dy Coordonnée destination y (colonne).
 * @param player Caractère du joueur qui bouge (ex 'b').
 * @param opponent Caractère de l'adversaire (ex 'r').
 * @param captured Tableau de sortie des coordonnées capturées.
 */

// int compute_captures(
//     char board[N][N],
//     int sx,
//     int sy,
//     int dx,
//     int dy,
//     char player,
//     char opponent,
//     int captured[MAX_CAP][2])
// {
//     int cap_count = 0;
//     char tmp_board[N][N];
//     memcpy(tmp_board, board, sizeof(tmp_board));

//     // Simulation
//     tmp_board[dx][dy] = tmp_board[sx][sy];
//     tmp_board[sx][sy] = '.';

//     const int dirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

//     // keep track to avoid double reporting same captured square
//     bool marked[N][N] = {false};

//     // 1) Linca (sandwich) detection - robust / principal mécanisme
//     for (int d = 0; d < 4; ++d)
//     {
//         int ax = dx + dirs[d][0];
//         int ay = dy + dirs[d][1];
//         int bx = ax + dirs[d][0];
//         int by = ay + dirs[d][1];

//         if (!in_board(ax, ay))
//             continue;
//         if (tmp_board[ax][ay] != opponent)
//             continue;
//         if (!in_board(bx, by))
//             continue;

//         // If beyond cell contains a friendly piece -> capture (Linca)
//         if (tmp_board[bx][by] == player)
//         {
//             if (!marked[ax][ay] && cap_count < MAX_CAP)
//             {
//                 captured[cap_count][0] = ax;
//                 captured[cap_count][1] = ay;
//                 marked[ax][ay] = true;
//                 ++cap_count;
//             }
//         }
//     }

//     // 2) Seultou (prise en poussant) - interprétation proposée
//     // For any adjacent opponent piece to the moved-to cell: if the cell on the opposite
//     // side of that opponent (in same direction) is empty, then we treat it as captured.
//     // We do not re-capture squares already captured by Linca (marked[][]).
//     for (int d = 0; d < 4; ++d)
//     {
//         int ax = dx + dirs[d][0];
//         int ay = dy + dirs[d][1];
//         int bx = ax + dirs[d][0];
//         int by = ay + dirs[d][1];

//         if (!in_board(ax, ay))
//             continue;
//         if (tmp_board[ax][ay] != opponent)
//             continue;
//         if (!in_board(bx, by))
//             continue;

//         // If beyond opponent is empty -> capture by pushing (Seultou)
//         if (tmp_board[bx][by] == '.' && !marked[ax][ay])
//         {
//             if (cap_count < MAX_CAP)
//             {
//                 captured[cap_count][0] = ax;
//                 captured[cap_count][1] = ay;
//                 marked[ax][ay] = true;
//                 ++cap_count;
//             }
//         }
//     }

//     return cap_count;
// }

// /* Exemple d'utilisation (pseudo):
// int main() {
//   char board[N][N] = { ... };
//   int captured[MAX_CAP][2];
//   int n = compute_captures(board, 4,4, 4,7, 'b', 'r', captured);
//   for (int i=0;i<n;++i) printf("captured: %d,%d\n", captured[i][0], captured[i][1]);
// }
// */
