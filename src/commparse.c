#include "../include/commparse.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>

static int parse_cell(const char *s, Cell *out) {
    if (!isalpha(s[0]) || !isdigit(s[1])) return -1;
    char col_char = toupper(s[0]);
    int col = col_char - 'A' + 1;
    int row = s[1] - '0';
    if (col < 1 || col > 9 || row < 1 || row > 9) return -1;

    out->col = col;
    out->row = row;
    return 0;
}

int parse_cell_range(const char *text, CellRange *out) {
    if (!text || !out) return -1;
    // Format: "A2:C2"
    size_t len = strlen(text);
    if (len < 5 || text[2] != ':') return -1;

    if (parse_cell(text, &out->from) != 0) return -1;
    if (parse_cell(text + 3, &out->to) != 0) return -1;

    return 0;
}