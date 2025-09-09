#include <ctype.h>
#include <string.h>
#include <stdio.h>

// Fonction pour parser une cellule individuelle
static int parse_cell(const char *s, int *col, int *row) {
    if (!isalpha(s[0]) || !isdigit(s[1])) return -1;
    char col_char = toupper(s[0]);
    *col = col_char - 'A' + 1;
    *row = s[1] - '0';
    if (*col < 1 || *col > 9 || *row < 1 || *row > 9) return -1;
    return 0;
}

// Parse une range et retourne un tableau [x1, y1, x2, y2]
int parse_cell_range_to_array(const char *text, int coordinates[4]) {
    if (!text || !coordinates) return -1;
    
    // Format: "A2:C2"
    size_t len = strlen(text);
    if (len < 5 || text[2] != ':') return -1;

    int col1, row1, col2, row2;
    
    if (parse_cell(text, &col1, &row1) != 0) return -1;
    if (parse_cell(text + 3, &col2, &row2) != 0) return -1;

    coordinates[0] = col1;
    coordinates[1] = row1;
    coordinates[2] = col2;
    coordinates[3] = row2;
    
    return 0;
}

// Fonction d'affichage pour format [[x1,y1],[x2,y2]]
void print_coordinates_2d_format(int coordinates[4]) {
    printf("[[%d,%d],[%d,%d]]\n", 
           coordinates[0], coordinates[1], 
           coordinates[2], coordinates[3]);
}

/*
int main() {
    const char *test_str = "A2:C2";
    int coords[4];
    
    if (parse_cell_range_to_array(test_str, coords) == 0) {
        print_coordinates_2d_format(coords);
    } else {
        printf("[[],[]]\n");
    }

    return 0;
}
*/