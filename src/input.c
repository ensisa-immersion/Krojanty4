#include <gtk/gtk.h>
#include <stdio.h>

#include "../include/input.h"
#include "../include/game.h"
#include "../include/display.h"

// Struct that bundles all the information needed to correctly predict input row and column
struct input_information
{
    Game *game;
    GtkWidget *drawing_area;
};

static void on_click(GtkGestureClick *gesture, int n_press, double x, double y, gpointer user_data)
{
    struct input_information *data = (struct input_information *)user_data;
    Game *game = data->game;
    GtkWidget *drawing_area = data->drawing_area;

    int width = gtk_widget_get_width(drawing_area);
    int height = gtk_widget_get_height(drawing_area);

    int col = (x - (width - GRID_SIZE * CELL_SIZE) / 2) / CELL_SIZE;
    int row = (y - (height - GRID_SIZE * CELL_SIZE) / 2) / CELL_SIZE;

    if (col >= 0 && col < GRID_SIZE && row >= 0 && row < GRID_SIZE)
    {
        printf("Clicked cell: %d, %d\n", row, col);
        printf("Click at %.1f, %.1f\n\n", x, y);
    }
}

void detect_click(GtkWidget *drawing_area, Game *game)
{
    // Creating and bundling all the data needed for on_click to work
    struct input_information *data = malloc(sizeof(struct input_information));
    data->game = game;
    data->drawing_area = drawing_area;

    // Create a "gesture", basically a listener for mouse clicks, that sends them as signals to on_click
    GtkGesture *click = gtk_gesture_click_new();
    gtk_widget_add_controller(drawing_area, GTK_EVENT_CONTROLLER(click));
    g_signal_connect(click, "pressed", G_CALLBACK(on_click), data);
}
