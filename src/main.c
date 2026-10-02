#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>

#define WIDTH 30
#define HEIGHT 10
#define ALIEN_COUNT 3
#define POINTS_PER_ALIEN 10

char gameField[HEIGHT][WIDTH];
int playerPos = WIDTH / 2;
int alienPos[ALIEN_COUNT][2]; // Each alien has x and y coordinates; y == -1 means destroyed
int bulletPos[2] = {-1, -1};  // Bullet x and y (-1 means no bullet)
int gameOver = 0;
int aliensRemaining = ALIEN_COUNT;
int score = 0;
int playerQuit = 0;

static struct termios origTermios;

// Restore the terminal settings saved in enableRawMode().
void disableRawMode(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &origTermios);
}

// Ctrl+C skips atexit handlers, so restore the terminal before exiting.
void handleSigint(int sig) {
    (void)sig;
    disableRawMode();
    _exit(1);
}

// Switch the terminal out of line-buffered (canonical) mode so each key
// press reaches the program immediately, without Enter and without echo.
void enableRawMode(void) {
    tcgetattr(STDIN_FILENO, &origTermios);
    atexit(disableRawMode);
    signal(SIGINT, handleSigint);

    struct termios raw = origTermios;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void initializeGame() {
    // Initialize the game field with spaces
    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            gameField[i][j] = ' ';
        }
    }

    // Place the player's ship on the bottom row
    gameField[HEIGHT - 1][playerPos] = '^';

    // Place the aliens on the second row, six columns apart
    for (int i = 0; i < ALIEN_COUNT; i++) {
        alienPos[i][0] = i * 6 + 1;
        alienPos[i][1] = 1;
        gameField[alienPos[i][1]][alienPos[i][0]] = 'A';
    }
}

void displayGame() {
    printf("\033[H\033[2J"); // ANSI escape: move cursor home and clear the screen
    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            putchar(gameField[i][j]);
        }
        putchar('\n');
    }
    printf("Score: %d   Aliens Remaining: %d\n", score, aliensRemaining);
    printf("Controls: 'a' = Left, 'd' = Right, 'space' = Shoot, 'q' = Quit\n");
    fflush(stdout);
}

void moveAliens() {
    static int direction = 1; // 1 = moving right, -1 = moving left
    static int moveDown = 0;  // Set when the formation hits a wall

    // Clear old alien positions
    for (int i = 0; i < ALIEN_COUNT; i++) {
        if (alienPos[i][1] >= 0) {
            gameField[alienPos[i][1]][alienPos[i][0]] = ' ';
        }
    }

    // If any live alien would leave the grid, reverse the formation and drop a row
    for (int i = 0; i < ALIEN_COUNT; i++) {
        if (alienPos[i][1] >= 0 && (alienPos[i][0] + direction < 0 || alienPos[i][0] + direction >= WIDTH)) {
            direction *= -1;
            moveDown = 1;
            break;
        }
    }

    // Move every live alien one step
    for (int i = 0; i < ALIEN_COUNT; i++) {
        if (alienPos[i][1] >= 0) {
            alienPos[i][0] += direction;
            if (moveDown) {
                alienPos[i][1]++;
                if (alienPos[i][1] == HEIGHT - 1) {
                    gameOver = 1; // Game over if aliens reach the player's row
                }
            }
        }
    }

    moveDown = 0;

    // Draw the aliens in their new positions
    for (int i = 0; i < ALIEN_COUNT; i++) {
        if (alienPos[i][1] >= 0 && alienPos[i][0] < WIDTH) {
            gameField[alienPos[i][1]][alienPos[i][0]] = 'A';
        }
    }
}

// If a live alien is at (x, y), destroy it, use up the bullet and return 1.
int hitAlienAt(int x, int y) {
    for (int i = 0; i < ALIEN_COUNT; i++) {
        if (alienPos[i][1] >= 0 && alienPos[i][0] == x && alienPos[i][1] == y) {
            gameField[y][x] = ' ';
            alienPos[i][1] = -1;
            bulletPos[1] = -1;
            score += POINTS_PER_ALIEN;
            aliensRemaining--;
            if (aliensRemaining == 0) {
                gameOver = 1; // Win the game if all aliens are destroyed
            }
            return 1;
        }
    }
    return 0;
}

void moveBullet() {
    if (bulletPos[1] == -1) {
        return; // No bullet active
    }

    int x = bulletPos[0];
    int y = bulletPos[1];

    // Clear the old bullet cell, but never erase an alien that moved into it
    if (gameField[y][x] == '|') {
        gameField[y][x] = ' ';
    }

    // Aliens move first, so an alien may have stepped into the bullet's cell
    if (hitAlienAt(x, y)) {
        return;
    }

    // Move the bullet up one row
    bulletPos[1]--;
    if (bulletPos[1] < 0) {
        bulletPos[1] = -1; // Bullet left the screen
        return;
    }

    // Did the bullet move into an alien?
    if (hitAlienAt(x, bulletPos[1])) {
        return;
    }

    gameField[bulletPos[1]][x] = '|';
}

void playerShoot() {
    if (bulletPos[1] == -1) { // Only one bullet can be active at a time
        bulletPos[0] = playerPos;
        bulletPos[1] = HEIGHT - 2; // Start just above the player's ship
    }
}

void handleInput(char input) {
    if (input == 'a' && playerPos > 0) {
        gameField[HEIGHT - 1][playerPos] = ' '; // Clear old position
        playerPos--;
        gameField[HEIGHT - 1][playerPos] = '^'; // Draw new position
    } else if (input == 'd' && playerPos < WIDTH - 1) {
        gameField[HEIGHT - 1][playerPos] = ' ';
        playerPos++;
        gameField[HEIGHT - 1][playerPos] = '^';
    } else if (input == ' ') {
        playerShoot();
    } else if (input == 'q') {
        playerQuit = 1;
        gameOver = 1;
    }
}

int main() {
    enableRawMode();
    initializeGame();

    while (!gameOver) {
        displayGame();
        moveAliens();
        moveBullet();

        // Wait up to 500 ms for a key without blocking the game loop
        char input;
        struct timeval timeout = {0, 500000};
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);

        if (select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout) > 0) {
            if (read(STDIN_FILENO, &input, 1) == 1) {
                handleInput(input);
            }
        }

        usleep(300000); // 300 ms between frames
    }

    displayGame(); // Show the final frame

    if (aliensRemaining == 0) {
        printf("You win! All aliens destroyed!\n");
    } else if (playerQuit) {
        printf("You quit.\n");
    } else {
        printf("Game Over! The aliens reached you!\n");
    }
    printf("Final score: %d\n", score);

    return 0;
}
