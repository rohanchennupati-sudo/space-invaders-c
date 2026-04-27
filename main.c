#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>

#define WIDTH 30
#define HEIGHT 10
#define ALIEN_COUNT 3  // Reduced number of aliens for easier gameplay

char gameField[HEIGHT][WIDTH];
int playerPos = WIDTH / 2;
int alienPos[ALIEN_COUNT][2]; // Each alien has x and y coordinates
int bulletPos[2] = {-1, -1}; // Bullet x and y (-1 means no bullet)
int gameOver = 0;
int aliensRemaining = ALIEN_COUNT;

void initializeGame() {
    // Initialize the game field with spaces
    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            gameField[i][j] = ' ';
        }
    }

    // Place the player's ship
    gameField[HEIGHT - 1][playerPos] = '^';

    // Initialize aliens with a slower movement
    for (int i = 0; i < ALIEN_COUNT; i++) {
        alienPos[i][0] = i * 6 + 1; // Spread aliens more evenly across the width
        alienPos[i][1] = 1;        // Start at the second row
        gameField[alienPos[i][1]][alienPos[i][0]] = 'A';
    }
}

void displayGame() {
    system("clear"); // Clear the console (works on Unix-like systems)
    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            printf("%c", gameField[i][j]);
        }
        printf("\n");
    }
    printf("Aliens Remaining: %d\n", aliensRemaining);
    printf("Controls: 'a' = Left, 'd' = Right, 'space' = Shoot\n");
}

void moveAliens() {
    static int direction = 1; // 1 = moving right, -1 = moving left
    static int moveDown = 0;  // Track when to move down

    // Clear old alien positions
    for (int i = 0; i < ALIEN_COUNT; i++) {
        if (alienPos[i][1] >= 0) {
            gameField[alienPos[i][1]][alienPos[i][0]] = ' ';
        }
    }

    // Check if any alien hits the wall
    for (int i = 0; i < ALIEN_COUNT; i++) {
        if (alienPos[i][1] >= 0 && (alienPos[i][0] + direction < 0 || alienPos[i][0] + direction >= WIDTH)) {
            direction *= -1; // Reverse direction
            moveDown = 1;    // Trigger downward movement
            break;
        }
    }

    // Move aliens slowly (slower alien movement for easier gameplay)
    for (int i = 0; i < ALIEN_COUNT; i++) {
        if (alienPos[i][1] >= 0) { // Only move active aliens
            alienPos[i][0] += direction; // Move left or right
            if (moveDown) {
                alienPos[i][1]++; // Move down
                if (alienPos[i][1] == HEIGHT - 1) {
                    gameOver = 1; // Game over if aliens reach the player's row
                }
            }
        }
    }

    moveDown = 0; // Reset moveDown flag

    // Update new alien positions
    for (int i = 0; i < ALIEN_COUNT; i++) {
        if (alienPos[i][1] >= 0 && alienPos[i][0] < WIDTH) { // Only draw active aliens
            gameField[alienPos[i][1]][alienPos[i][0]] = 'A';
        }
    }
}

void moveBullet() {
    if (bulletPos[1] == -1) {
        return; // No bullet active
    }

    // Clear old bullet position
    gameField[bulletPos[1]][bulletPos[0]] = ' ';

    // Move bullet upward faster (increase bullet speed)
    bulletPos[1] -= 1; // Bullet speed increased to make it faster

    if (bulletPos[1] < 0) {
        bulletPos[1] = -1; // Reset bullet if it goes off the screen
        return;
    }

    // Check for collision with aliens
    for (int i = 0; i < ALIEN_COUNT; i++) {
        if (alienPos[i][1] >= 0 && alienPos[i][0] == bulletPos[0] && alienPos[i][1] == bulletPos[1]) {
            gameField[alienPos[i][1]][alienPos[i][0]] = ' '; // Remove alien
            alienPos[i][1] = -1; // Alien is destroyed (inactive)
            bulletPos[1] = -1;   // Reset bullet
            aliensRemaining--;
            if (aliensRemaining == 0) {
                gameOver = 1; // Win the game if all aliens are destroyed
            }
            return;
        }
    }

    // Draw bullet in the new position
    gameField[bulletPos[1]][bulletPos[0]] = '|';
}

void playerShoot() {
    if (bulletPos[1] == -1) { // Shoot only if no bullet is active
        bulletPos[0] = playerPos;
        bulletPos[1] = HEIGHT - 2; // Start just above the player's ship
    }
}

void handleInput(char input) {
    if (input == 'a' && playerPos > 0) {
        gameField[HEIGHT - 1][playerPos] = ' '; // Clear old position
        playerPos--;                            // Move left
        gameField[HEIGHT - 1][playerPos] = '^'; // Draw new position
    } else if (input == 'd' && playerPos < WIDTH - 1) {
        gameField[HEIGHT - 1][playerPos] = ' '; // Clear old position
        playerPos++;                            // Move right
        gameField[HEIGHT - 1][playerPos] = '^'; // Draw new position
    } else if (input == ' ') {
        playerShoot();
    }
}

int main() {
    initializeGame();

    while (!gameOver) {
        displayGame();
        moveAliens();
        moveBullet();

        // Check for user input
        char input;
        struct timeval timeout = {0, 500000}; // 500ms delay for slower gameplay
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);

        if (select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout)) {
            read(STDIN_FILENO, &input, 1);
            handleInput(input);
        }

        usleep(300000); // Reduced game loop speed to make gameplay slower
    }

    if (aliensRemaining == 0) {
        printf("You win! All aliens destroyed!\n");
    } else {
        printf("Game Over! The aliens reached you!\n");
    }

    return 0;
}