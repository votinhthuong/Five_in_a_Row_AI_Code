#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAX_CLIENTS 2
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8888
#define BOARD_SIZE 15

typedef struct {
    int board[BOARD_SIZE][BOARD_SIZE];
    int current_player;
    char player1_nickname[50];
    char player2_nickname[50];
    int player1_socket;
    int player2_socket;
} GameState;

void initialize_board(int board[BOARD_SIZE][BOARD_SIZE]) {
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            board[i][j] = 0;
        }
    }
}

int is_valid_move(int board[BOARD_SIZE][BOARD_SIZE], int row, int col) {
    if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) {
        return 0;
    }
    if (board[row][col] != 0) {
        return 0;
    }
    return 1;
}

void place_piece(int board[BOARD_SIZE][BOARD_SIZE], int row, int col, int player) {
    board[row][col] = player;
}

int check_winner(int board[BOARD_SIZE][BOARD_SIZE], int row, int col, int player) {
    // Check horizontal
    int count = 0;
    for (int i = 0; i < BOARD_SIZE; i++) {
        if (board[row][i] == player) {
            count++;
            if (count == 5) {
                return 1;
            }
        } else {
            count = 0;
        }
    }

    // Check vertical
    count = 0;
    for (int i = 0; i < BOARD_SIZE; i++) {
        if (board[i][col] == player) {
            count++;
            if (count == 5) {
                return 1;
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
        if (board[i][j] == player) {
            count++;
            if (count == 5) {
                return 1;
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
        if (board[i][j] == player) {
            count++;
            if (count == 5) {
                return 1;
            }
        } else {
            count = 0;
        }
        i--;
        j++;
    }

    return 0;
}

void serialize_game_state(GameState *game_state, char *buffer) {
    sprintf(buffer, "%s|%s|%d", game_state->player1_nickname, game_state->player2_nickname, game_state->current_player);
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            sprintf(buffer + strlen(buffer), "|%d", game_state->board[i][j]);
        }
    }
}

void deserialize_game_state(char *buffer, GameState *game_state) {
    char *token;
    token = strtok(buffer, "|");
    strcpy(game_state->player1_nickname, token);
    token = strtok(NULL, "|");
    strcpy(game_state->player2_nickname, token);
    token = strtok(NULL, "|");
    game_state->current_player = atoi(token);
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            token = strtok(NULL, "|");
            game_state->board[i][j] = atoi(token);
        }
    }
}

void send_game_state(GameState *game_state) {
    char buffer[2048];
    serialize_game_state(game_state, buffer);
    send(game_state->player1_socket, buffer, strlen(buffer), 0);
    send(game_state->player2_socket, buffer, strlen(buffer), 0);
}

void handle_move(GameState *game_state, int client_socket, char *buffer) {
    int row, col;
    sscanf(buffer, "%d,%d", &row, &col);  // Parse row and column from the received buffer

    if (client_socket == game_state->player1_socket && game_state->current_player == 1) {
        if (is_valid_move(game_state->board, row, col)) {
            place_piece(game_state->board, row, col, 1);
            if (check_winner(game_state->board, row, col, 1)) {
                send_game_state(game_state);
                send(game_state->player1_socket, "WIN", 3, 0);
                send(game_state->player2_socket, "LOSE", 4, 0);
            } else {
                game_state->current_player = 2;
                send_game_state(game_state);
            }
        } else {
            send(game_state->player1_socket, "INVALID_MOVE", 12, 0);
        }
    } else if (client_socket == game_state->player2_socket && game_state->current_player == 2) {
        if (is_valid_move(game_state->board, row, col)) {
            place_piece(game_state->board, row, col, 2);
            if (check_winner(game_state->board, row, col, 2)) {
                send_game_state(game_state);
                send(game_state->player2_socket, "WIN", 3, 0);
                send(game_state->player1_socket, "LOSE", 4, 0);
            } else {
                game_state->current_player = 1;
                send_game_state(game_state);
            }
        } else {
            send(game_state->player2_socket, "INVALID_MOVE", 12, 0);
        }
    }
}

void *handle_client(void *arg) {
    int client_socket = *(int *)arg;
    GameState *game_state = NULL;
    char buffer[1024];

    // Find the game state associated with the client socket
    if (client_socket == game_state->player1_socket) {
        game_state = (GameState *)arg;
    } else if (client_socket == game_state->player2_socket) {
        game_state = (GameState *)arg;
    }

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            break;
        }

        handle_move(game_state, client_socket, buffer);
    }

    close(client_socket);
    free(arg);
    pthread_exit(NULL);
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_address, client_address;
    pthread_t thread_id;
    GameState game_state;

    // Create a socket for the server
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    // Set up the server address structure
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT);
    server_address.sin_addr.s_addr = inet_addr(SERVER_IP);

    // Bind the socket to the server address
    bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address));

    // Listen for incoming connections
    listen(server_socket, MAX_CLIENTS);

    while (1) {
        // Accept client connections
        socklen_t client_address_length = sizeof(client_address);
        client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_length);

        if (game_state.player1_socket == 0) {
            game_state.player1_socket = client_socket;
            initialize_board(game_state.board);
            game_state.current_player = 1;
            strcpy(game_state.player1_nickname, "Player 1");
            send(game_state.player1_socket, "1", 1, 0);
            printf("Player 1 connected\n");
        } else if (game_state.player2_socket == 0) {
            game_state.player2_socket = client_socket;
            strcpy(game_state.player2_nickname, "Player 2");
            send(game_state.player2_socket, "2", 1, 0);
            printf("Player 2 connected\n");

            // Create separate threads for each player
            int *player1_socket = malloc(sizeof(int));
            *player1_socket = game_state.player1_socket;
            pthread_create(&thread_id, NULL, handle_client, (void *)player1_socket);

            int *player2_socket = malloc(sizeof(int));
            *player2_socket = game_state.player2_socket;
            pthread_create(&thread_id, NULL, handle_client, (void *)player2_socket);
        }
    }

    return 0;
}