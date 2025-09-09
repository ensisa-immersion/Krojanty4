#ifndef COMMPARSE_H
#define COMMPARSE_H

// Parse une range et retourne un tableau [x1, y1, x2, y2]
// coordinates doit être un tableau de 4 entiers alloué par l'appelant
// Retourne 0 en cas de succès, -1 en cas d'erreur
int parse_cell_range_to_array(const char *text, int coordinates[4]);

// Fonction utilitaire pour afficher les coordonnées au format [[x1,y1],[x2,y2]]
void print_coordinates_2d_format(int coordinates[4]);

#endif // COMMPARSE_H