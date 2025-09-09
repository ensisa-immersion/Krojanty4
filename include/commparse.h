#ifndef CELL_RANGE_H
#define CELL_RANGE_H

typedef struct {
    int col;  // 1 to 9 (A=1, ..., I=9)
    int row;  // 1 to 9
} Cell;

typedef struct {
    Cell from;
    Cell to;
} CellRange;

// Parse a range string like "A2:C2" or "E4:E8" into a CellRange struct.
// Returns 0 on success, -1 on error (bad format).
int parse_cell_range(const char *text, CellRange *out);

#endif // CELL_RANGE_H