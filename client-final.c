#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>

#define BOARD_SIZE 15
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8888

typedef struct {
    int board[BOARD_SIZE][BOARD_SIZE];
    int player_num;
    int current_player;
    char player_nickname[50];
    char opponent_nickname[50];
    int socket;
    GtkWidget *drawing_area;
} GameState;

void initialize_board(int board[BOARD_SIZE][BOARD_SIZE]) {
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            board[i][j] = 0;
        }
    }
}

gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer data) {
    GameState *game_state = (GameState *)data;
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
            if (game_state->board[i][j] != 0) {
                if (game_state->board[i][j] == game_state->player_num) {
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

gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data) {
    GameState *game_state = (GameState *)data;

    if (event->button == GDK_BUTTON_PRIMARY) {
        GtkAllocation allocation;
        gtk_widget_get_allocation(widget, &allocation);

        int width = allocation.width;
        int height = allocation.height;

        int row = (int)(event->y * BOARD_SIZE / height);
        int col = (int)(event->x * BOARD_SIZE / width);

        if (game_state->board[row][col] == 0 && game_state->player_num == game_state->current_player) {
            char buffer[50];
            sprintf(buffer, "%d,%d", row, col);
            send(game_state->socket, buffer, strlen(buffer), 0);
        }
    }

    return TRUE;
}

void update_game_state(GameState *game_state, char *buffer) {
    char *token;
    token = strtok(buffer, "|");
    strcpy(game_state->player_nickname, token);
    token = strtok(NULL, "|");
    strcpy(game_state->opponent_nickname, token);
    token = strtok(NULL, "|");
    game_state->current_player = atoi(token);
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            token = strtok(NULL, "|");
            game_state->board[i][j] = atoi(token);
        }
    }

    gtk_widget_queue_draw(game_state->drawing_area);
}

void *receive_messages(void *arg) {
    GameState *game_state = (GameState *)arg;
    char buffer[1024];

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(game_state->socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            break;
        }

        if (strcmp(buffer, "INVALID_MOVE") == 0) {
            printf("Invalid move. Please try again.\n");
        } else if (strcmp(buffer, "WIN") == 0) {
            printf("Congratulations! You won the game.\n");
            break;
        } else if (strcmp(buffer, "LOSE") == 0) {
            printf("Sorry, you lost the game.\n");
            break;
        } else {
            update_game_state(game_state, buffer);
        }
    }

    gtk_main_quit();
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    GameState game_state;
    initialize_board(game_state.board);

    gtk_init(&argc, &argv);

    // Create the main window
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Five-in-a-Row");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 400);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Create the drawing area
    game_state.drawing_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(game_state.drawing_area, 400, 400);
    gtk_widget_add_events(game_state.drawing_area, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(game_state.drawing_area, "draw", G_CALLBACK(on_draw), &game_state);
    g_signal_connect(game_state.drawing_area, "button-press-event", G_CALLBACK(on_button_press), &game_state);

    gtk_container_add(GTK_CONTAINER(window), game_state.drawing_area);

    // Connect to the server
    game_state.socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT);
    server_address.sin_addr.s_addr = inet_addr(SERVER_IP);
    connect(game_state.socket, (struct sockaddr *)&server_address, sizeof(server_address));

    // Receive player number
    char buffer[50];
    recv(game_state.socket, buffer, sizeof(buffer), 0);
    game_state.player_num = atoi(buffer);

    // Start receiving messages from the server
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, receive_messages, (void *)&game_state);

    gtk_widget_show_all(window);
    gtk_main();

    // Clean up
    close(game_state.socket);

    return 0;
}