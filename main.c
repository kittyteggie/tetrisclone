#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define ROWS 20
#define COLS 10

static struct termios term_original;
void restore_terminal(void) {
  tcsetattr(STDIN_FILENO, TCSANOW, &term_original);
  printf("\033[?25h");
  fflush(stdout);
}
void setup_terminal(void) {
  struct termios term_raw;
  tcgetattr(STDIN_FILENO, &term_original);
  term_raw = term_original;
  term_raw.c_lflag &= ~(ICANON | ECHO);
  term_raw.c_cc[VMIN] = 0;
  term_raw.c_cc[VTIME] = 0;
  tcsetattr(STDIN_FILENO, TCSANOW, &term_raw);
  int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
  printf("\033[?25l");
  atexit(restore_terminal);
}
typedef struct {
  const char *rotations[4];
} Piece;
static const Piece PIECES[7] = {{{"0000"
                                  "1111"
                                  "0000"
                                  "0000",
                                  "0010"
                                  "0010"
                                  "0010"
                                  "0010",
                                  "0000"
                                  "0000"
                                  "1111"
                                  "0000",
                                  "0100"
                                  "0100"
                                  "0100"
                                  "0100"}},
                                {{"0000"
                                  "0110"
                                  "0110"
                                  "0000",
                                  "0000"
                                  "0110"
                                  "0110"
                                  "0000",
                                  "0000"
                                  "0110"
                                  "0110"
                                  "0000",
                                  "0000"
                                  "0110"
                                  "0110"
                                  "0000"}},
                                {{"0000"
                                  "1110"
                                  "0100"
                                  "0000",
                                  "0100"
                                  "1100"
                                  "0100"
                                  "0000",
                                  "0000"
                                  "0100"
                                  "1110"
                                  "0000",
                                  "0100"
                                  "0110"
                                  "0100"
                                  "0000"}},
                                {{"0000"
                                  "0110"
                                  "1100"
                                  "0000",
                                  "0100"
                                  "0110"
                                  "0010"
                                  "0000",
                                  "0000"
                                  "0110"
                                  "1100"
                                  "0000",
                                  "0100"
                                  "0110"
                                  "0010"
                                  "0000"}},
                                {{"0000"
                                  "1100"
                                  "0110"
                                  "0000",
                                  "0010"
                                  "0110"
                                  "0100"
                                  "0000",
                                  "0000"
                                  "1100"
                                  "0110"
                                  "0000",
                                  "0010"
                                  "0110"
                                  "0100"
                                  "0000"}},
                                {{"1000"
                                  "1110"
                                  "0000"
                                  "0000",
                                  "0110"
                                  "0100"
                                  "0100"
                                  "0000",
                                  "0000"
                                  "1110"
                                  "0010"
                                  "0000",
                                  "0100"
                                  "0100"
                                  "1100"
                                  "0000"}},
                                {{"0010"
                                  "1110"
                                  "0000"
                                  "0000",
                                  "0100"
                                  "0100"
                                  "0110"
                                  "0000",
                                  "0000"
                                  "1110"
                                  "1000"
                                  "0000",
                                  "1100"
                                  "0100"
                                  "0100"
                                  "0000"}}};

int board[ROWS][COLS];
typedef struct {
  int type;
  int rotation;
  int x, y;
} ActivePiece;
ActivePiece current;
int score = 0;
int lines = 0;
int game_over = 0;
int piece_cell(int type, int rotation, int row, int col) {
  return PIECES[type].rotations[rotation][row * 4 + col] == '1';
}
int check_collision(int type, int rotation, int px, int py) {
  for (int r = 0; r < 4; r++) {
    for (int c = 0; c < 4; c++) {
      if (!piece_cell(type, rotation, r, c))
        continue;
      int tx = px + c;
      int ty = py + r;
      if (tx < 0 || tx >= COLS || ty >= ROWS)
        return 1;
      if (ty >= 0 && board[ty][tx] != 0)
        return 1;
    }
  }
  return 0;
}
void spawn_piece(void) {
  current.type = rand() % 7;
  current.rotation = 0;
  current.x = COLS / 2 - 2;
  current.y = -1;
  if (check_collision(current.type, current.rotation, current.x, current.y)) {
    game_over = 1;
  }
}
void lock_piece(void) {
  for (int r = 0; r < 4; r++) {
    for (int c = 0; c < 4; c++) {
      if (piece_cell(current.type, current.rotation, r, c)) {
        int tx = current.x + c;
        int ty = current.y + r;
        if (ty >= 0 && ty < ROWS && tx >= 0 && tx < COLS) {
          board[ty][tx] = current.type + 1;
        }
      }
    }
  }
}
void clear_lines(void) {
  int cleared = 0;
  for (int r = ROWS - 1; r >= 0; r--) {
    int full = 1;
    for (int c = 0; c < COLS; c++) {
      if (board[r][c] == 0) {
        full = 0;
        break;
      }
    }
    if (full) {
      cleared++;
      for (int rr = r; rr > 0; rr--) {
        memcpy(board[rr], board[rr - 1], sizeof(board[0]));
      }
      memset(board[0], 0, sizeof(board[0]));
      r++;
    }
  }
  if (cleared > 0) {
    int points[5] = {0, 100, 300, 500, 800};
    score += points[cleared];
    lines += cleared;
  }
}
int move_piece(int dx, int dy) {
  if (!check_collision(current.type, current.rotation, current.x + dx,
                       current.y + dy)) {
    current.x += dx;
    current.y += dy;
    return 1;
  }
  return 0;
}
void rotate_piece(void) {
  int new_rot = (current.rotation + 1) % 4;
  if (!check_collision(current.type, new_rot, current.x, current.y)) {
    current.rotation = new_rot;
  } else if (!check_collision(current.type, new_rot, current.x - 1,
                              current.y)) {
    current.x -= 1;
    current.rotation = new_rot;
  } else if (!check_collision(current.type, new_rot, current.x + 1,
                              current.y)) {
    current.x += 1;
    current.rotation = new_rot;
  }
}
void draw(void) {
  char buffer[ROWS][COLS];
  memset(buffer, 0, sizeof(buffer));
  for (int r = 0; r < ROWS; r++)
    for (int c = 0; c < COLS; c++)
      buffer[r][c] = board[r][c] != 0 ? 1 : 0;
  for (int r = 0; r < 4; r++) {
    for (int c = 0; c < 4; c++) {
      if (piece_cell(current.type, current.rotation, r, c)) {
        int ty = current.y + r;
        int tx = current.x + c;
        if (ty >= 0 && ty < ROWS && tx >= 0 && tx < COLS) {
          buffer[ty][tx] = 1;
        }
      }
    }
  }
  printf("\033[H");
  printf("SIMPLE TETRIS\r\n");
  printf("Score: %d   Lines: %d\r\n", score, lines);
  printf("+");
  for (int c = 0; c < COLS; c++)
    printf("--");
  printf("+\r\n");
  for (int r = 0; r < ROWS; r++) {
    printf("|");
    for (int c = 0; c < COLS; c++) {
      printf("%s", buffer[r][c] ? "[]" : "  ");
    }
    printf("|\r\n");
  }
  printf("+");
  for (int c = 0; c < COLS; c++)
    printf("--");
  printf("+\r\n");
  printf("Controls: a/d move, s drop, w/space rotate, q quit     \r\n");
  fflush(stdout);
}
int main(void) {
  setup_terminal();
  srand((unsigned int)time(NULL));
  memset(board, 0, sizeof(board));
  spawn_piece();
  printf("\033[2J");
  struct timespec last_tick, now;
  clock_gettime(CLOCK_MONOTONIC, &last_tick);
  double drop_interval = 0.5;
  while (!game_over) {
    char key;
    while (read(STDIN_FILENO, &key, 1) == 1) {
      switch (key) {
      case 'a':
      case 'A':
        move_piece(-1, 0);
        break;
      case 'd':
      case 'D':
        move_piece(1, 0);
        break;
      case 's':
      case 'S':
        move_piece(0, 1);
        break;
      case 'w':
      case 'W':
      case ' ':
        rotate_piece();
        break;
      case 'q':
      case 'Q':
        game_over = 1;
        break;
      }
    }
    clock_gettime(CLOCK_MONOTONIC, &now);
    double elapsed = (now.tv_sec - last_tick.tv_sec) +
                     (now.tv_nsec - last_tick.tv_nsec) / 1e9;
    if (elapsed >= drop_interval) {
      last_tick = now;
      if (!move_piece(0, 1)) {
        lock_piece();
        clear_lines();
        spawn_piece();
        drop_interval = 0.5 - (lines / 10) * 0.04;
        if (drop_interval < 0.1)
          drop_interval = 0.1;
      }
    }
    draw();
    usleep(16000);
  }
  printf("\033[2J\033[H");
  printf("Game Over.\r\n");
  printf("Final Score: %d\r\n", score);
  printf("Lines Cleared: %d\r\n", lines);
  return 0;
}
