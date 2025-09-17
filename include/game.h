/**
 * @file game.h
 * @brief Structures et fonctions principales du moteur de jeu
 * @author Équipe IMM2526-GR4
 * @date 17 septembre 2025
 * 
 * Ce fichier contient les définitions fondamentales du jeu, incluant :
 * - Les énumérations pour les joueurs, pièces et directions
 * - La structure principale Game contenant l'état du jeu
 * - Les fonctions de gestion des règles et de la logique de jeu
 * - Les fonctions d'initialisation et de mise à jour du plateau
 * - L'API utilisée par les différents modules (IA, réseau, interface)
 */

#ifndef GAME_H_INCLUDED
#define GAME_H_INCLUDED
#include <time.h>

/**
 * @enum GameMode
 * @brief Modes de jeu disponibles
 * 
 * Définit les différents modes d'exécution du jeu selon le type de partie.
 */
typedef enum {
    LOCAL = 0,  /**< Jeu local (deux joueurs sur le même ordinateur) */
    SERVER,     /**< Mode serveur (attente de connexions clients) */
    CLIENT      /**< Mode client (connexion à un serveur distant) */
} GameMode;

/**
 * @enum Player
 * @brief Identifiants des joueurs et états de partie
 * 
 * Énumération définissant les joueurs actifs et les états de fin de partie.
 */
typedef enum {
    NOT_PLAYER = 0, /**< Aucun joueur (case vide ou état neutre) */
    P1,             /**< Joueur 1 (généralement les pièces bleues) */
    P2,             /**< Joueur 2 (généralement les pièces rouges) */
    DRAW            /**< Match nul (égalité entre les joueurs) */
} Player;

/**
 * @enum Piece
 * @brief Types de pièces et états des cases du plateau
 * 
 * Définit tous les types de pièces possibles sur le plateau ainsi que
 * les états spéciaux des cases (vides, visitées).
 */
typedef enum {
    P_NONE = 0,   /**< Case vide */
    P1_PAWN,      /**< Pion du joueur 1 */
    P2_PAWN,      /**< Pion du joueur 2 */
    P1_KING,      /**< Roi du joueur 1 */
    P2_KING,      /**< Roi du joueur 2 */
    P1_VISITED,   /**< Case précédemment occupée par le joueur 1 */
    P2_VISITED    /**< Case précédemment occupée par le joueur 2 */
} Piece;

/**
 * @enum Direction
 * @brief Directions de mouvement sur le plateau
 * 
 * Énumération des directions cardinales possibles pour les mouvements
 * des pièces sur le plateau de jeu.
 */
typedef enum {
    DIR_TOP,    /**< Direction vers le haut */
    DIR_DOWN,   /**< Direction vers le bas */
    DIR_LEFT,   /**< Direction vers la gauche */
    DIR_RIGHT,  /**< Direction vers la droite */
    NONE        /**< Aucune direction (état neutre) */
} Direction;

/**
 * @struct Game
 * @brief Structure principale contenant l'état complet du jeu
 * 
 * Cette structure centralise toutes les informations nécessaires pour
 * représenter l'état d'une partie en cours, incluant le plateau,
 * les informations de tour, et les paramètres de jeu.
 */
typedef struct {
    int won;                /**< État de victoire (Player: NOT_PLAYER, P1, P2, ou DRAW) */
    int turn;               /**< Numéro du tour actuel (commence à 0) */
    int selected_tile[2];   /**< Coordonnées [ligne, colonne] de la case sélectionnée */
    int is_ai;              /**< Flag indiquant si l'IA est activée (1 = oui, 0 = non) */
    time_t turn_timer;      /**< Timestamp du début du tour actuel */
    
    GameMode game_mode;     /**< Mode de jeu actuel (LOCAL, SERVER, CLIENT) */
    Piece board[9][9];      /**< Plateau de jeu 9x9 contenant les pièces */
} Game;

// ============================================================================
// FONCTIONS DE VÉRIFICATION DES RÈGLES DU JEU
// ============================================================================

/**
 * @brief Vérifie et met à jour l'état de victoire du jeu
 * 
 * Cette fonction examine l'état actuel du plateau pour déterminer si un joueur
 * a gagné selon les règles du jeu. Elle met à jour le champ 'won' de la structure
 * Game avec le résultat (P1, P2, DRAW, ou NOT_PLAYER si la partie continue).
 * 
 * @param game Pointeur vers la structure de jeu à examiner
 * @return void
 */
void won(Game* game);

/**
 * @brief Gère les captures de pièces après un mouvement
 * 
 * Cette fonction vérifie les cases adjacentes à la position donnée pour détecter
 * et appliquer les captures selon les règles du jeu. Elle est appelée après
 * chaque mouvement pour mettre à jour l'état du plateau.
 * 
 * @param game Pointeur vers la structure de jeu à modifier
 * @param row Ligne de la position où vérifier les captures
 * @param col Colonne de la position où vérifier les captures
 * @param sprint_direction Direction du mouvement effectué (influence les captures)
 * @return void
 */
void did_eat(Game* game, int row, int col, Direction sprint_direction);

/**
 * @brief Vérifie si un mouvement est légal selon les règles
 * 
 * Cette fonction valide un mouvement proposé en vérifiant toutes les règles
 * du jeu : appartenance de la pièce, validité du trajet, absence d'obstacles,
 * et conformité aux règles de déplacement.
 * 
 * @param game Pointeur vers la structure de jeu
 * @param src_row Ligne de la case source
 * @param src_col Colonne de la case source
 * @param dst_row Ligne de la case destination
 * @param dst_col Colonne de la case destination
 * @return int 1 si le mouvement est légal, 0 sinon
 */
int is_move_legal(Game* game, int src_row, int src_col, int dst_row, int dst_col);

/**
 * @brief Détermine le joueur propriétaire d'une pièce
 * 
 * Cette fonction utilitaire extrait l'information de joueur à partir du type
 * de pièce. Utile pour identifier rapidement à qui appartient une pièce
 * sur le plateau.
 * 
 * @param piece Type de pièce à examiner
 * @return Player Joueur propriétaire (P1, P2, ou NOT_PLAYER)
 */
Player get_player(Piece piece);

// ============================================================================
// FONCTIONS DE STATISTIQUES ET SCORES
// ============================================================================

/**
 * @brief Calcule le score actuel du joueur 1
 * 
 * Cette fonction compte et évalue les pièces du joueur 1 présentes sur le plateau
 * pour calculer son score selon les règles de comptage établies.
 * 
 * @param game Structure de jeu à analyser (passée par valeur)
 * @return int Score du joueur 1
 */
int score_player_one(Game game);

/**
 * @brief Calcule le score actuel du joueur 2
 * 
 * Cette fonction compte et évalue les pièces du joueur 2 présentes sur le plateau
 * pour calculer son score selon les règles de comptage établies.
 * 
 * @param game Structure de jeu à analyser (passée par valeur)
 * @return int Score du joueur 2
 */
int score_player_two(Game game);

// ============================================================================
// API PRINCIPALE UTILISÉE PAR LES MODULES EXTERNES
// ============================================================================

/**
 * @brief Met à jour le plateau de jeu avec un mouvement
 * 
 * Cette fonction centrale applique un mouvement sur le plateau en déplaçant
 * la pièce sélectionnée vers la destination, en gérant les captures, et en
 * mettant à jour tous les états associés (tour, victoire, etc.).
 * 
 * @param game Pointeur vers la structure de jeu à modifier
 * @param dst_row Ligne de destination du mouvement
 * @param dst_col Colonne de destination du mouvement
 * @return void
 */
void update_board(Game* game, int dst_row, int dst_col);

/**
 * @brief Initialise une nouvelle partie
 * 
 * Cette fonction crée et configure une nouvelle instance de jeu avec les
 * paramètres spécifiés. Elle initialise le plateau, configure le mode de jeu,
 * et prépare tous les états nécessaires pour commencer une partie.
 * 
 * @param mode Mode de jeu à utiliser (LOCAL, SERVER, CLIENT)
 * @param artificial_intelligence Flag d'activation de l'IA (1 = activée, 0 = désactivée)
 * @return Game Structure de jeu initialisée et prête à l'emploi
 */
Game init_game(GameMode mode, int artificial_intelligence);

/**
 * @brief Détermine quel joueur doit jouer actuellement
 * 
 * Cette fonction calcule le joueur actuel basé sur le numéro de tour.
 * Utile pour l'interface utilisateur et la logique de contrôle de tour.
 * 
 * @param game Pointeur vers la structure de jeu
 * @return Player Joueur dont c'est le tour (P1 ou P2)
 */
Player current_player_turn(Game* game);


#endif // GAME_H_INCLUDED
