#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>     // for rand()
#include <time.h>       // for seeding srand() and nanosleep()
#include <string.h>     // for GameObjects images
#include <signal.h>     // for signals

#include <termios.h>    // for struct termios   for getche() from conio.h
#include <unistd.h>     // for STDIN_FILENO     for getche() from conio.h
#include <sys/wait.h>   // for waitpid()


// - - - SETTING GLOBAL VARIABLES
bool GAME_RUNNING;
// sets width and height of the game area
const int WIDTH = 40;
const int HEIGHT = 31;
// and its components separetely
const int SKY_HEIGHT = 1;
const int START_AMOUNT_FISH = 3;
// Game controls instruction
char INSTRUCTION[] = "Press the 'A' and 'D' buttons to move the\n\tboat left and right accordingly. Press 'E'\n\tto exit.";

int AAA = 0, B = 0;
// - - - INITIALIZING STRUCTURES
struct GameObject {
    unsigned int length : 2;
    int posX,
        posY,
        speed,
        direction;
    char image[3];
};
struct Fish {
    struct GameObject obj;
    bool isCatched;
};

struct Boat {
    struct GameObject obj;
    bool isFishing;
};


// declaring functions
void fillGameMap(int gameMap[HEIGHT][WIDTH], struct Fish fishes[], struct Boat);
void drawGame(int gameMap[HEIGHT][WIDTH], struct Fish fishes[], struct Boat);
void endGame();
void moveBoat(struct Boat boat);
void moveFish(struct Fish fishes[]);
void turnFish(struct Fish fishes[], int);
void handleUserInput(int, struct Boat);
void handleSignalFromChild();
int getUserInput(struct timespec);
int getche();


int main() {
    int gameMap[HEIGHT][WIDTH];
    int keyPressedNumber;
    // water depth divided by amount of fishes for their future uniform distribution (and to ensure that fishes will not spawn one insise another)
    int waterPart = (HEIGHT - SKY_HEIGHT) / START_AMOUNT_FISH;

    // seeds rand()
    srand(time(NULL));

    // creates boat and sets the x index of the rounded centre minus 1 of the game area as its x pos.
    struct Boat boat;
    boat.isFishing = false;
    boat.obj.length = 3;
    boat.obj.posX = WIDTH / 2 - 1;
    strcpy(boat.obj.image, "\\_/");

    // creates START_AMOUNT_FISH of fishes
    struct Fish fishes[START_AMOUNT_FISH];
    for (int i=0; i < START_AMOUNT_FISH; i++) {
        // const class values
        fishes[i].isCatched = false;
        fishes[i].obj.length = 3;
        fishes[i].obj.speed = 1;
        // random values
        fishes[i].obj.posX = rand() % (WIDTH - fishes[i].obj.length);
        fishes[i].obj.posY = rand() % waterPart + SKY_HEIGHT + waterPart * i;
        fishes[i].obj.direction = rand()%2 == 0 ? -1 : 1; // -1 or 1 for left and right
        if (fishes[i].obj.direction == 1) {
            strcpy(fishes[i].obj.image, "><>");
        }
        else {
            strcpy(fishes[i].obj.image, "<><");
        }
    }

    // declares ts timespec structure for nanosleep()
    // ..and sets the frame rate (tv_nsec value)
    struct timespec ts;
    ts.tv_sec = 0;          // 0 s
    ts.tv_nsec = 300000000; // 300 ms


    GAME_RUNNING = true;
    while (GAME_RUNNING) {

        fillGameMap(gameMap, fishes, boat);
        drawGame(gameMap, fishes, boat);

        // gets number of the key user pressed, 0 if none
        keyPressedNumber = getUserInput(ts);

        // reacts accordingly to the input provided by the user
        handleUserInput(keyPressedNumber, boat);

        // moves boat (if boat.obj.direction != 0)
        moveBoat(boat);
        // moves each fish
        moveFish(fishes);

        /*boat moves left-> gameMap[0][boat.posX-1]='︵' and
         *                  gameMap[0][boat.posX+boat.length+1]='‿'*/
    }

    return 0;
}

//- - - fills the gameMap
void fillGameMap(int gameMap[HEIGHT][WIDTH], struct Fish fishes[], struct Boat boat) {
    // 1. Add waves
    // gameMap[0][WIDTH] {0};    /*0 stands for waves*/
    for (int x=0; x < WIDTH; x++)
        gameMap[0][x] = 0;    /*0 stands for waves*/

    // 2. Add boat
    for (int l=0; l < boat.obj.length; l++) {
        gameMap[0][boat.obj.posX+l] = 1;  /*1 stands for boat*/
    }

    // 3. Add water
    for (int y=SKY_HEIGHT; y < HEIGHT; y++) {
        for (int x=0; x < WIDTH; x++) gameMap[y][x] = 2;    /*2 stands for water*/
    }

    // 4. Add fishes
    for (int i=0; i < START_AMOUNT_FISH; i++) {
        for (int j=0; j < fishes[i].obj.length; j++)
            gameMap[fishes[i].obj.posY][fishes[i].obj.posX + j] = 3;    /*3 stands for fish*/
    }
}

// - - - draws the game screen
void drawGame(int gameMap[HEIGHT][WIDTH], struct Fish fishes[], struct Boat boat) {
    // clears the console
    printf("\e[1;1H\e[2J");
    // prints INSTRUCTION
    printf("\t%s\n", INSTRUCTION);

    // printf("\n\t");  /* splits the instruction into equal lines (==WIDTH) */
    // for (int i = 0; i < strlen(INSTRUCTION); i++) {
    //     if ((i + 1) % WIDTH == 0)
    //         printf("\n\t");
    //     printf("%c", INSTRUCTION[i]);
    // }

    // printf("drawGame is started in %d time\n", AAA++);
    // for (int i=0; i < START_AMOUNT_FISH; i++) {
    //     printf("Fish %d posX: %d, direction: %d\n", i+1, fishes[i].obj.posX, fishes[i].obj.direction);
    // }

    int fishNum = 0;
    // skips 4 lines as an indent
    printf("\n\n\n\n");
    // iterates through the game map and checks what sign must be drawn
    for (int y=0; y < HEIGHT; y++) {
        printf("\t|");
        for (int x=0; x < WIDTH; x++) {
            // можно попробовать такой вариант с массивом объектов
            // printf("%", gameObjects[gameMap[y][x]]);

            switch (gameMap[y][x]) {
                // wave
                case 0:
                    printf("_");
                    break;
                // boat
                case 1:
                    // for (int i=0; i < boat.obj.length; i++) {
                    //     printf("%c", boat.obj.image[i]);
                    // }
                    printf("%s", boat.obj.image);
                    x += boat.obj.length - 1;
                    break;
                // water
                case 2:
                    printf("░");
                    break;
                // fish
                case 3:
                    // for (int i=0; i < fishes[0].obj.length; i++) {
                    //     printf("%c", fishes[0].obj.image[i]);
                    // }
                    printf("%s", fishes[fishNum].obj.image);
                    fishNum++;
                    x += fishes[0].obj.length - 1;
                    break;
                // hook
                case 4:
                    printf("ʖ");
                    break;
                // fishing line
                case 5:
                    printf("|");
                    break;
            }
        }
        printf("|\n");
    }
    // skips 4 lines as an indent
    printf("\n\n\n\n");
}

// turns around the fish with the index i
void turnFish(struct Fish fishes[], int index) {
    if (fishes[index].obj.direction == 1) {
        fishes[index].obj.posX = WIDTH - fishes[index].obj.length;
        fishes[index].obj.direction = -1;
        strcpy(fishes[index].obj.image, "<><");
    }
    else {
        fishes[index].obj.posX = 0;
        fishes[index].obj.direction = 1;
        strcpy(fishes[index].obj.image, "><>");
    }
}

void moveFish(struct Fish fishes[]) {
    for (int i=0; i < START_AMOUNT_FISH; i++) {
        // move fishes that haven't been catched yet
        if (!fishes[i].isCatched) {
            fishes[i].obj.posX += fishes[i].obj.direction * \
                                    fishes[i].obj.speed;
            // if fish crosses one of the borders
            if (fishes[i].obj.posX < 0 || fishes[i].obj.posX > WIDTH - fishes[i].obj.length - 1) {
                // turns fish and corrects its x position
                turnFish(fishes, i);
            }
        }
    }
}

void moveBoat(struct Boat boat) {
    // sets new position of the boat
    boat.obj.posX += boat.obj.direction * boat.obj.speed;
    // corrects boat position
    if (boat.obj.posX < 0) {
        boat.obj.posX = 0;
    }
    else if (boat.obj.posX > WIDTH - boat.obj.length) {
        boat.obj.posX = WIDTH - boat.obj.length;
    }
}

void handleUserInput(int keyPressedNumber, struct Boat boat) {
    // printf("> handleUserInput() started\n\n\n");
    switch (keyPressedNumber) {
        case 97:                        /* 'a' key */
            boat.obj.direction = -1;
            break;
        case 100:                       /* 'd' key */
            boat.obj.direction = 1;
            break;
        case 101:                       /* 'e' key */
            // clears the console
            printf("\e[1;1H\e[2J");
            printf("'E' button was pressed. Exiting.\nThank you for playing my game :)\n\n(c) Leucist - https://github.com/Leucist\n\n");
            GAME_RUNNING = false;
            break;
        default:                        /* None or other key was pressed */
            boat.obj.direction = 0;     /* idle */
    }
}

void endGame() {
    // clears the console
    printf("\e[1;1H\e[2J");
    printf("'E' button was pressed. Exiting.\nThank you for playing my game :)\n\n");
    exit(1);
}

int getUserInput(struct timespec ts) {
    // B++;
    // printf("> getUserInput() started for %d time\n", B);

    // id of child process that cathes the user keydown
    int catcherId, childExitStatus, keyPressedNumber = 0;
    if ((catcherId = fork()) < 0) {}    /*well, it can catch an error~*/
    if (catcherId == 0) {               /* child process */
        // gets the key pressed by user
        keyPressedNumber = getche();
        exit(keyPressedNumber);
    }
    else {                              /* parent process */
        nanosleep(&ts, &ts);
        // checks if child process executed and gather related data
        waitpid(catcherId, &childExitStatus, WNOHANG);
        // sets child's exit status as keyPressedNumber
        // ..if exited normally, otherwise key value is set to 0
        keyPressedNumber = WIFEXITED(childExitStatus) ? WEXITSTATUS(childExitStatus) : 0;
        // kills child process (even if it already exited)
        kill(catcherId, SIGQUIT);
    }

    return keyPressedNumber;
}

// reads a single character from the keyboard and returns its value immediately without waiting for enter key (conio.h)
int getche(){
    struct termios oldt, newt;
    int ch;
    tcgetattr( STDIN_FILENO, &oldt );
    newt = oldt;
    newt.c_lflag &= ~ICANON;
    newt.c_lflag &=  ECHO;
    tcsetattr( STDIN_FILENO, TCSANOW, &newt );
    ch = getchar();
    tcsetattr( STDIN_FILENO, TCSANOW, &oldt );
    return ch;
}
