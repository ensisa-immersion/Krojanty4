#include <gtk/gtk.h>
#include "input.h"
#include "game.h"
#include "display.h"

// Struct that bundles all the information needed to correctly predict input row and column
struct input_information {
    Game* game;
    GtkWidget* drawing_area;
};


static void on_click(GtkGestureClick *gesture, int n_press, double x, double y, gpointer user_data) {
    struct input_information *data = (struct input_information *)user_data;
    Game *game = data->game;
    GtkWidget *drawing_area = data->drawing_area;

    // Compute row/col from click
    int width  = gtk_widget_get_width(drawing_area);
    int height = gtk_widget_get_height(drawing_area);

    int col = (x - (width - GRID_SIZE * CELL_SIZE) / 2) / CELL_SIZE;
    int row = (y - (height - GRID_SIZE * CELL_SIZE) / 2) / CELL_SIZE;

    if (col < 0 || col >= GRID_SIZE || row < 0 || row >= GRID_SIZE) return;

    int selected_row = game->selected_tile[0];
    int selected_col = game->selected_tile[1];

    // If nothing selected, select a piece owned by current player
    if (selected_row == -1 || selected_col == -1) {
        if (game->board[row][col] != P_NONE) {
            if ((game->turn % 2 == 0 && (game->board[row][col] == P1_PAWN || game->board[row][col] == P1_KING)) ||
                (game->turn % 2 == 1 && (game->board[row][col] == P2_PAWN || game->board[row][col] == P2_KING)))
            {
                game->selected_tile[0] = row;
                game->selected_tile[1] = col;
            }
        }
        return;
    }

    // A piece is selected: try to move
    update_board(game, row, col);

    // Redraw the board
    gtk_widget_queue_draw(drawing_area);
}


void detect_click(GtkWidget *drawing_area, Game *game) {
    // Creating and bundling all the data needed for on_click to work
    struct input_information *data = malloc(sizeof(struct input_information));
    data->game = game;
    data->drawing_area = drawing_area;

    // Create a "gesture", basically a listener for mouse clicks, that sends them as signals to on_click
    GtkGesture *click = gtk_gesture_click_new();
    gtk_widget_add_controller(drawing_area, GTK_EVENT_CONTROLLER(click));
    g_signal_connect(click, "pressed", G_CALLBACK(on_click), data);
}
