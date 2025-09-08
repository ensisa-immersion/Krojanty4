#include <string.h>

/**
 * Valide si les coordonnées sont dans le format attendu.
 * @param col Colonne (A-I).
 * @param row Ligne (1-9).
 * @return 1 si valide, 0 sinon.
 */
int coord_valide(char col, char row) {
    return (col >= 'A' && col <= 'I' && row >= '1' && row <= '9');
}

/**
 * Valide l'entrée du joueur.
 * @param buffer Chaîne de caractères contenant l'entrée.
 * @return 1 si l'entrée est valide, 0 sinon.
 */
int entree_valide(const char *buffer) {
    // Format attendu: "A2:A3"
    if (strlen(buffer) != 5 || buffer[2] != ':')
        return 0;
    return coord_valide(buffer[0], buffer[1]) && coord_valide(buffer[3], buffer[4]);
}


/**
 * Vérifie si le déplacement est diagonal (interdit).
 * @param buffer Chaîne de caractères contenant l'entrée.
 * @return 1 si le déplacement n'est pas diagonal, 0 sinon.
 */
int deplacement_diagonale_interdit(const char *buffer) {
    int col1 = buffer[0] - 'A';
    int row1 = buffer[1] - '1';
    int col2 = buffer[3] - 'A';
    int row2 = buffer[4] - '1';
    // Déplacement diagonal si les deux coordonnées changent
    return !((col1 != col2) && (row1 != row2));
}
