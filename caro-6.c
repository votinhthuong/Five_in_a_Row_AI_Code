// Include necessary libraries
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>

// Define game constants
#define BOARD_SIZE 15
#define PLAYER_1 1
#define PLAYER_2 2

// Define game data structures
int board[BOARD_SIZE][BOARD_SIZE];
int current_player = PLAYER_1;

// Function prototypes
void initialize_board();
int is_valid_move(int row, int col);
void place_piece(int row, int col);
int check_winner(int row, int col);
void switch_player();
gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer data);
gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data);
void start_new_game(GtkWidget *widget, gpointer data);
void quit_game(GtkWidget *widget, gpointer data);

// Main function
int main(int argc, char *argv[]) {
    GtkWidget *window;
    GtkWidget *drawing_area;
    GtkWidget *button_box;
    GtkWidget *new_game_button;
    GtkWidget *quit_button;

    gtk_init(&argc, &argv);

    // Create the main window
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Five-in-a-Row");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 400);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Create the drawing area
    drawing_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(drawing_area, 400, 400);
    gtk_widget_add_events(drawing_area, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(drawing_area, "draw", G_CALLBACK(on_draw), NULL);
    g_signal_connect(drawing_area, "button-press-event", G_CALLBACK(on_button_press), NULL);

    // Create the button box
    button_box = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(button_box), GTK_BUTTONBOX_END);

    // Create the "New Game" button
    new_game_button = gtk_button_new_with_label("New Game");
    g_signal_connect(new_game_button, "clicked", G_CALLBACK(start_new_game), drawing_area);
    gtk_container_add(GTK_CONTAINER(button_box), new_game_button);

    // Create the "Quit" button
    quit_button = gtk_button_new_with_label("Quit");
    g_signal_connect(quit_button, "clicked", G_CALLBACK(quit_game), NULL);
    gtk_container_add(GTK_CONTAINER(button_box), quit_button);

    // Add the drawing area and button box to the window
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(vbox), drawing_area, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), button_box, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    initialize_board();

    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}

// Function to initialize the game board
void initialize_board() {
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            board[i][j] = 0;
        }
    }
}

// Function to check if a move is valid
int is_valid_move(int row, int col) {
    if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) {
        return 0;
    }
    if (board[row][col] != 0) {
        return 0;
    }
    return 1;
}

// Function to place a piece on the board
void place_piece(int row, int col) {
    board[row][col] = current_player;
}

// Function to check for a winning condition
int check_winner(int row, int col) {
    // Check horizontal
    int count = 0;
    for (int i = 0; i < BOARD_SIZE; i++) {
        if (board[row][i] == current_player) {
            count++;
            if (count == 5) {
                return current_player;
            }
        } else {
            count = 0;
        }
    }

    // Check vertical
    count = 0;
    for (int i = 0; i < BOARD_SIZE; i++) {
        if (board[i][col] == current_player) {
            count++;
            if (count == 5) {
                return current_player;
            }
        } else {
            count = 0;
        }
    }

    // Check diagonal (top-left to bottom-right)
    count = 0;
    int i = row - col;
    int j = 0;
    if (i < 0) {
        j = -i;
        i = 0;
    }
    while (i < BOARD_SIZE && j < BOARD_SIZE) {
        if (board[i][j] == current_player) {
            count++;
            if (count == 5) {
                return current_player;
            }
        } else {
            count = 0;
        }
        i++;
        j++;
    }

    // Check diagonal (top-right to bottom-left)
    count = 0;
    i = row + col;
    j = 0;
    if (i >= BOARD_SIZE) {
        j = i - BOARD_SIZE + 1;
        i = BOARD_SIZE - 1;
    }
    while (i >= 0 && j < BOARD_SIZE) {
        if (board[i][j] == current_player) {
            count++;
            if (count == 5) {
                return current_player;
            }
        } else {
            count = 0;
        }
        i--;
        j++;
    }

    return 0;
}

// Function to switch the current player
void switch_player() {
    current_player = (current_player == PLAYER_1) ? PLAYER_2 : PLAYER_1;
}

// Function to handle drawing the game board
// Function to handle drawing the game board
gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer data) {
    guint width, height;
    GdkRGBA color;
    GtkStyleContext *context;

    context = gtk_widget_get_style_context(widget);
    width = gtk_widget_get_allocated_width(widget);
    height = gtk_widget_get_allocated_height(widget);

    gtk_render_background(context, cr, 0, 0, width, height);

    // Draw the grid lines
    cairo_set_line_width(cr, 1);
    cairo_set_source_rgb(cr, 0, 0, 0);

    for (int i = 0; i <= BOARD_SIZE; i++) {
        cairo_move_to(cr, i * width / BOARD_SIZE, 0);
        cairo_line_to(cr, i * width / BOARD_SIZE, height);
        cairo_move_to(cr, 0, i * height / BOARD_SIZE);
        cairo_line_to(cr, width, i * height / BOARD_SIZE);
    }

    cairo_stroke(cr);

    // Draw the pieces
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (board[i][j] != 0) {
                if (board[i][j] == PLAYER_1) {
                    cairo_set_source_rgb(cr, 1, 0, 0);
                } else {
                    cairo_set_source_rgb(cr, 0, 0, 1);
                }

                cairo_arc(cr, (j + 0.5) * width / BOARD_SIZE, (i + 0.5) * height / BOARD_SIZE, 10, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    }

    return FALSE;
}


// Function to handle mouse clicks on the drawing area
gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data) {
    if (event->button == GDK_BUTTON_PRIMARY) {
        GtkAllocation allocation;
        gtk_widget_get_allocation(widget, &allocation);

        int width = allocation.width;
        int height = allocation.height;

        int col = (int)(event->x * BOARD_SIZE / width);
        int row = (int)(event->y * BOARD_SIZE / height);

        if (is_valid_move(row, col)) {
            place_piece(row, col);

            if (check_winner(row, col)) {
                char message[50];
                snprintf(message, sizeof(message), "Player %d wins!", current_player);
                GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(widget)),
                                                           GTK_DIALOG_DESTROY_WITH_PARENT,
                                                           GTK_MESSAGE_INFO,
                                                           GTK_BUTTONS_OK,
                                                           "%s", message);
                gtk_dialog_run(GTK_DIALOG(dialog));
                gtk_widget_destroy(dialog);
                initialize_board();
            } else {
                switch_player();
            }

            gtk_widget_queue_draw(widget);
        }
    }

    return TRUE;
}

// Function to start a new game
void start_new_game(GtkWidget *widget, gpointer data) {
    initialize_board();
    current_player = PLAYER_1;
    gtk_widget_queue_draw(GTK_WIDGET(data));
}

// Function to quit the game
void quit_game(GtkWidget *widget, gpointer data) {
    gtk_main_quit();
}