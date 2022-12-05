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
char PLAYER[10] = "player";
const int TOP_SIZE = 20;    // actual amount*2~
// sets width and height of the game area
const int WIDTH = 40;
const int HEIGHT = 31;
// and its components separetely
const int SKY_HEIGHT = 1;
int START_AMOUNT_FISH = 5;
// Game controls instruction
char INSTRUCTION[] = "> How to play?\n\t- Hold the 'A' and 'D' buttons to\n\tmove the boat left and right accordingly,\n\tpress 'F' to throw down a hook. If it touches a fish - you catch it!\n\t-Press 'E' to exit.";
char CONTROLS[] = "'a', 'd' - Move boat left, right\n\t'f' - Throw/pull the hook\n\t'e' - Exit game";
char HELP_CONTROLS[] = "[Hold 'h' to show the controls]\n\n";
char MESSAGE[80];

// - - - INITIALIZING STRUCTURES
struct GameObject {
    // unsigned int length : 2;
    int posX,
        posY,
        speed,
        direction,
        length;
    char image[3];
};
struct Fish {
    struct GameObject obj;
    bool isCaught;
};

struct Boat {
    struct GameObject obj;
    int hookPosY,
        hookSpeed,
        hookDirection;
    bool isFishing,
         hookDragFish;
};

struct Button {
    unsigned int crntBtn : 2; /* quick workaround~ :P */
    int length,
        posY;
    bool isChosen;
    char name[8];
};


// declaring functions
void start();
void menu(bool*);
void game();
void menuButtonHandler(struct Button buttons[], int btnsAmount, bool*);
void records(char, int);
void fillGameMap(int gameMap[HEIGHT][WIDTH], struct Fish fishes[], struct Boat);
void drawGame(int gameMap[HEIGHT][WIDTH], struct Fish fishes[], struct Boat, int score, int timer);
void moveHook(struct Boat*, int *score);
void moveBoat(struct Boat*);
void moveFish(struct Fish fishes[]);
void turnFish(struct Fish fishes[], int);
void handleUserInput(int, struct Boat*);
void checkFishCaught(struct Boat*, struct Fish fishes[]);
int getUserInput(struct timespec);
int getche();


int main() {
    start();
    bool exitGame = false;
    while (!exitGame) {
        menu(&exitGame);
    }
    return 0;
}

void start() {
    // clears the console
    printf("\e[1;1H\e[2J");
    printf("--------^----------------Please, adjust your terminal size so \n\t|\t\tthat you can see both ends of the arrow.\n");
    for (int i = 0; i < HEIGHT+8; i++) {
        if (i == HEIGHT / 2 - 1) {
            printf("\t|\t\t[ If you see both arrow ends\t]\n");
            continue;
        }
        else if (i == HEIGHT / 2) {
            printf("\t|\t\t[ screen is adjusted properly.\t]\n");
            continue;
        }
        else if (i == HEIGHT / 2 + 1) {
            printf("\t|\t\t[ Press any button to continue.\t]\n");
            continue;
        }
        printf("\t|\n");
    }
    printf("--------v---------------- - end of the arrow. Now you see it :D But do you see the start?\n");
    if (getche()) records('c', 0);  /* creates records file if none */
}

void drawMenuHeader() {
    // clears the console
    printf("\e[1;1H\e[2J");
    printf("\t[ Use up and down arrows to switch menu options ]\n");
    // skips 4 lines as an indent
    printf("\n\n\n\n");
    // draws the game menu
    printf("\t|");
    for (int x=0; x < WIDTH; x++) {
        if (x == WIDTH / 2 - 1) {
            printf("\\_/");
            x += 3;
        }
        printf("_");
    }
    printf("|\n");
}

void menu(bool *exitGame) {
    int keyPressedNumber, btnsAmount = 4;
    char gap;
    // initialises buttons
    struct Button buttons[btnsAmount];
    buttons[0].crntBtn = 0;
    // init btn start
    strcpy(buttons[0].name, "START");
    buttons[0].posY = HEIGHT / 2 - 4;
    buttons[0].length = 5;
    buttons[0].isChosen = true;
    // init btn records
    strcpy(buttons[1].name, "RECORDS");
    buttons[1].posY = HEIGHT / 2 - 2;
    buttons[1].length = 7;
    buttons[1].isChosen = false;
    // init btn settings
    strcpy(buttons[2].name, "SETTINGS");
    buttons[2].posY = HEIGHT / 2;
    buttons[2].length = 8;
    buttons[2].isChosen = false;
    // init btn exit
    strcpy(buttons[3].name, "EXIT");
    buttons[3].posY = HEIGHT / 2 + 2;
    buttons[3].length = 4;
    buttons[3].isChosen = false;

    int nextBtn;
    while (!*exitGame) {
        nextBtn = 0;
        drawMenuHeader();
        for (int y=1; y < HEIGHT; y++) {
            printf("\t|");
            // skips line if it doesn't contain the next button
            if (y != buttons[nextBtn].posY) {
                for (int x=0; x < WIDTH; x++) printf("░");
            }
            else {
                for (int x=0; x < WIDTH; x++) {
                    if (x == WIDTH / 2 - 4) {
                        for (int i = 0; i < btnsAmount; i++) {
                            if (y == buttons[i].posY) {
                                if (buttons[i].isChosen) printf(">");
                                else printf("░");
                                printf("%s", buttons[i].name);
                                if (buttons[i].isChosen) printf("<");
                                else printf("░");
                                x += buttons[i].length+2;
                            }
                        }
                    }
                    printf("░");
                }
                nextBtn++;
            }
            printf("|\n");
        }
        keyPressedNumber = getche();
        if (keyPressedNumber == 10) menuButtonHandler(buttons, btnsAmount, exitGame);
        else if (keyPressedNumber == 27 && getche() == 91) {
            switch(getche()) {
                case 65:                        /* up arrow */
                    buttons[0].crntBtn -= 1;
                    break;
                case 66:                        /* down arrow */
                    buttons[0].crntBtn += 1;
                    break;
            }
            for (int i = 0; i < btnsAmount; i++) buttons[i].isChosen = false;
            buttons[buttons[0].crntBtn].isChosen = true;
        }
    }
}

void settings() {
    // int middle = (HEIGHT - SKY_HEIGHT) / 2 - 2,
    //     sLineNum = 0;
    // bool settingsRunning = true;
    // while (settingsRunning) {
    //     drawMenuHeader();
    //     for (int y=1; y < HEIGHT; y++) {
    //         printf("\t|");
    //         if (y != middle + sLineNum * 2) {
    //             for (int x=0; x < WIDTH; x++) printf("░");
    //         }
    //         else {
    //             for (int x=0; x < WIDTH; x++) {
    //                 if (x == WIDTH / 2 - 13) {
    //                     printf("|%2d.%10s %10s|", rlineNum/2+1, rline[rlineNum], rline[rlineNum+1]);
    //                     rlineNum = rlineNum < (TOP_SIZE - 2) ? rlineNum+=2 : 0;
    //                     x += 26;
    //                 }
    //                 printf("░");
    //             }
    //             sLineNum++;
    //         }
    //         printf("|\n");
    //     }
    //
    // }
}

void menuButtonHandler(struct Button buttons[], int btnsAmount, bool *exitGame) {
    if (buttons[0].isChosen) {
        game();
    }
    else if (buttons[1].isChosen) {
        records('r', 0);
    }
    else if (buttons[2].isChosen) {
        settings();
    }
    else if (buttons[3].isChosen) {
        *exitGame = true;
    }
}

void drawRecords(FILE *recordsFile, char rline[TOP_SIZE][10]) {
    int rlineNum = 0;
    int middle = (HEIGHT - SKY_HEIGHT) / 2 - TOP_SIZE / 4;
    // while () {} well, there was a while loop. Redundant without anim's
    drawMenuHeader();
    for (int y=1; y < HEIGHT; y++) {
        printf("\t|");
        // if (y == middle - 1) {
        //     printf("|Pos:        Time score:|");
        // }
        // skips line if it doesn't contain the record line
        if (y != middle + rlineNum / 2) {
            for (int x=0; x < WIDTH; x++) printf("░");
        }
        else {
            for (int x=0; x < WIDTH; x++) {
                if (x == WIDTH / 2 - 13) {
                    fscanf(recordsFile, "%s", rline[rlineNum]);
                    fscanf(recordsFile, "%s", rline[rlineNum+1]);
                    printf("|%2d.%10s %10s|", rlineNum/2+1, rline[rlineNum], rline[rlineNum+1]);
                    rlineNum = rlineNum < (TOP_SIZE - 2) ? rlineNum+=2 : 0;
                    x += 26;
                }
                printf("░");
            }
        }
        printf("|\n");
    }
    printf("\tPress any key to return to the menu >\n");
    getche();
}

void records(char fmode, int timer) {
    FILE *recordsFile;
    char rline[TOP_SIZE][10];
    if (fmode == 'c') {
        recordsFile = fopen("records.txt", "a");
        fclose(recordsFile);
        recordsFile = fopen("records.txt", "r+");
        if (getc(recordsFile) == EOF) {
            for (int i = 0; i < TOP_SIZE; i++) {
                switch(i%2) {
                    case 0:
                        fprintf(recordsFile, "...\n");
                        break;
                    case 1:
                        fprintf(recordsFile, "0\n");
                        break;
                }
            }
        }
        fclose(recordsFile);
        return;
    }
    recordsFile = fopen("records.txt", "r");
    if (fmode == 'r') {
        drawRecords(recordsFile, rline);
    }
    // changes leaderboard if needed
    else if (fmode == 'w') {
        int offset;
        bool recordsCorrected = false;
        for (int i = 0; i < TOP_SIZE; i++) {
            offset = 0;
            fscanf(recordsFile, "%s", rline[i]);
            // if it's the timeValue line
            if (i%2==1 && !recordsCorrected){
                // if prev.record is slower than a new one or r.is empty
                if (atoi(rline[i]) > timer || rline[i][0] == '0') {
                    if (i != TOP_SIZE - 1) {
                        // moves down the previous leader
                        strcpy(rline[i+1], rline[i-1]);
                        strcpy(rline[i+2], rline[i]);
                        offset = 2;
                    }
                    // inserts new data
                    strcpy(rline[i-1], PLAYER);
                    sprintf(rline[i], "%d", timer);
                    recordsCorrected = true;
                    i += offset;
                }
            }
        }
        fclose(recordsFile);
        recordsFile = fopen("records.txt", "w");
        for (int i = 0; i < TOP_SIZE; i++) {
            printf("%s\n", rline[i]);
            fprintf(recordsFile, "%s\n", rline[i]);
        }
        fclose(recordsFile);
    }
}

void game() {
    int gameMap[HEIGHT][WIDTH];
    int keyPressedNumber, timer=0, score=0;
    // water depth divided by amount of fishes for their future uniform distribution (and to ensure that fishes will not spawn one insise another)
    int waterPart = (HEIGHT - SKY_HEIGHT) / START_AMOUNT_FISH;
    time_t timerStart;

    // seeds rand()
    srand(time(NULL));

    // creates boat and sets the x index of the rounded centre minus 1 of the game area as its x pos.
    struct Boat boat;
    boat.isFishing      = false;
    boat.hookDragFish   = false;    /* doesn't drag any fish */
    boat.hookDirection  = 0;        /* hook doesn't move */
    boat.hookPosY       = 0;
    boat.hookSpeed      = 1;
    boat.obj.speed      = 1;
    boat.obj.length     = 3;
    boat.obj.posX = WIDTH / 2 - 1;
    strcpy(boat.obj.image, "\\_/");

    // creates START_AMOUNT_FISH of fishes
    struct Fish fishes[START_AMOUNT_FISH];
    for (int i=0; i < START_AMOUNT_FISH; i++) {
        // const class values
        fishes[i].isCaught = false;
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

    timerStart = time(NULL);
    strcpy(MESSAGE, HELP_CONTROLS);
    GAME_RUNNING = true;

    while (GAME_RUNNING) {
        fillGameMap(gameMap, fishes, boat);
        drawGame(gameMap, fishes, boat, score, timer);

        // gets number of the key user pressed, 0 if none
        keyPressedNumber = getUserInput(ts);

        // reacts accordingly to the input provided by the user
        handleUserInput(keyPressedNumber, &boat);

        // moves boat (if boat.obj.direction != 0)
        moveBoat(&boat);
        // moves each fish
        moveFish(fishes);
        // calculates the game time spent
        timer = time(NULL) - timerStart;

        // fish if fishing :)
        if (boat.isFishing) {
            moveHook(&boat, &score);
            checkFishCaught(&boat, fishes);
            // if player wins
            if (score == START_AMOUNT_FISH) {
                nanosleep(&ts, &ts);
                printf("\e[1;1H\e[2J");
                printf("CONGRATULATIONS!\nYou won :D\n\n");
                // may be fancy edited later, for now uses a quick hack~
                printf("Type your name: ");
                scanf("%10s", &PLAYER);

                records('w', timer);

                GAME_RUNNING = false;
            }
        }
    }
    printf("\nThank you for playing my game, %s :)\n\n(c) Leucist - https://github.com/Leucist\n\n", PLAYER);
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
        // if fish wasn't caught
        if (!fishes[i].isCaught) {
            for (int j=0; j < fishes[i].obj.length; j++)
                gameMap[fishes[i].obj.posY][fishes[i].obj.posX + j] = 3;    /*3 stands for fish*/
        }
    }

    // Add waves when boat is moving
    if (boat.obj.direction == 1 && boat.obj.posX < WIDTH - boat.obj.length) {
        gameMap[0][boat.obj.posX+boat.obj.length] = 6;
        // assumming that posX can't be zero:
        gameMap[0][boat.obj.posX-1] = 7;
    }
    else if (boat.obj.direction == -1 && boat.obj.posX > 0) {
        gameMap[0][boat.obj.posX+boat.obj.length] = 7;
        gameMap[0][boat.obj.posX-1] = 6;
    }

    // Add hook and line if fishing
    if (boat.hookPosY != 0) {
        int hookPosX = boat.obj.posX+1;
        // adds line above the hook
        for (int y = 1; y < boat.hookPosY; y++)
            gameMap[y][hookPosX] = 5;
        // adds hook itself
        gameMap[boat.hookPosY][hookPosX] = 4;
    }
}

// - - - draws the game screen
void drawGame(int gameMap[HEIGHT][WIDTH], struct Fish fishes[], struct Boat boat, int score, int timer) {
    // clears the console
    printf("\e[1;1H\e[2J");
    // prints INSTRUCTION
    printf("\t%s\n", MESSAGE);
    printf("\t> Score: %d\t\t\tTime: %ds.", score, timer);

    int fishNum = 0;
    // skips 4 lines as an indent
    printf("\n\n\n\n");
    // iterates through the game map and checks what sign must be drawn
    for (int y=0; y < HEIGHT; y++) {
        printf("\t|");
        if (y == SKY_HEIGHT + ((HEIGHT - SKY_HEIGHT) / START_AMOUNT_FISH) * (fishNum + 1)) fishNum++;
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
                    x += fishes[0].obj.length - 1;
                    break;
                // hook
                case 4:
                    if (!boat.hookDragFish)
                        printf("ʖ");
                    else
                        printf("&");
                    break;
                // fishing line
                case 5:
                    printf("|");
                    break;

                // wave before boat
                case 6:
                    printf("‿");
                    break;
                // wave after boat
                case 7:
                    printf("‿");
                    break;
            }
        }
        printf("|\n");
    }
    // skips 4 lines as an indent
    printf("\n\n\n\n");
}

void checkFishCaught(struct Boat *boat, struct Fish fishes[]) {
    int *hookPosY       = &boat -> hookPosY,
        *hookSpeed      = &boat -> hookSpeed,
        *hookDirection  = &boat -> hookDirection;
    bool *hookDragFish  = &boat -> hookDragFish;
    for (int i=0; i < START_AMOUNT_FISH; i++) {
        if (fishes[i].isCaught) continue;
        // if fish faces the
        // <left, right, into, under> - fishPos : hookPos

        // if fish is above the hook
        if (fishes[i].obj.posY < boat->hookPosY) {
            if (fishes[i].obj.posX + fishes[i].obj.length == boat -> obj.posX+1 ||
            fishes[i].obj.posX + fishes[i].obj.length-2 == boat -> obj.posX ||
            fishes[i].obj.posX == boat->obj.posX+1 ||
            fishes[i].obj.posX == boat->obj.posX) {
                turnFish(fishes, i);
            }
        }
        // if fish is on the same y as hook
        if (fishes[i].obj.posY == boat->hookPosY) {
            // if fish collides with the hook
            for (int l = 0; l < fishes[i].obj.length; l++) {
                if (fishes[i].obj.posX + l == boat->obj.posX+1) {
                    fishes[i].isCaught = true;
                    fishes[i].obj.direction = 0;
                    *hookDragFish = true;
                    *hookDirection = -1;
                    // *hookPosY += *hookSpeed;
                }
            }
        }
    }
}

// turns around the fish with the index i
void turnFish(struct Fish fishes[], int id) {
    if (fishes[id].obj.direction == 1) {
        fishes[id].obj.direction = -1;
        strcpy(fishes[id].obj.image, "<><");
    }
    else {
        fishes[id].obj.direction = 1;
        strcpy(fishes[id].obj.image, "><>");
    }
}

void moveFish(struct Fish fishes[]) {
    for (int i=0; i < START_AMOUNT_FISH; i++) {
        // move fishes that haven't been caught yet
        if (!fishes[i].isCaught) {
            fishes[i].obj.posX += fishes[i].obj.direction * \
                                    fishes[i].obj.speed;
            // if fish crosses one of the borders
            if (fishes[i].obj.posX < 0) {
                // turns fish and corrects its x position
                fishes[i].obj.posX = 0;
                turnFish(fishes, i);
            }
            else if (fishes[i].obj.posX > WIDTH - fishes[i].obj.length - 1) {
                // turns fish and corrects its x position
                fishes[i].obj.posX = WIDTH - fishes[i].obj.length;
                turnFish(fishes, i);
            }
        }
    }
}

void moveHook(struct Boat *boat, int *score) {
    int *hookPosY       = &boat -> hookPosY,
        *hookSpeed      = &boat -> hookSpeed,
        *hookDirection  = &boat -> hookDirection;
    bool *isFishing     = &boat -> isFishing,
         *hookDragFish  = &boat -> hookDragFish;
    // moves the hook
    *hookPosY += (*hookSpeed) * (*hookDirection);
    // correct hook's position(+)
    if (*hookPosY >= (HEIGHT - 1)) {
        *hookPosY = HEIGHT - 1;
        *hookDirection = -1;
    }
    else if (*hookPosY <= 0) {
        if (*hookDragFish) *score += 1;
        *isFishing = false;
        *hookDragFish = false;
        *hookPosY = 0;
        *hookDirection = 0;     /* redundant operation, but just in case */
    }
}

void moveBoat(struct Boat *boat) {
    int *posX       =   &boat -> obj.posX,
        *speed      =   &boat -> obj.speed,
        *length     =   &boat -> obj.length,
        *direction  =   &boat -> obj.direction;
    // sets new position of the boat
    *posX += *direction * *speed;
    // corrects boat position
    if (*posX < 0) {
        *posX = 0;
    }
    else if (*posX > WIDTH - *length) {
        *posX = WIDTH - *length;
    }
}

void handleUserInput(int keyPressedNumber, struct Boat *boat) {
    int *direction      =   &boat -> obj.direction,
        *hookDirection  =   &boat -> hookDirection;
    bool *isFishing     =   &boat -> isFishing;
    switch (keyPressedNumber) {
        case 97:                        /* 'a' key */
            if (!(boat->isFishing)) {
                *direction = -1;
            }
            break;
        case 100:                       /* 'd' key */
            if (!(boat->isFishing)) {
                *direction = 1;
            }
            break;
        case 101:                       /* 'e' key */
            // clears the console
            printf("\e[1;1H\e[2J");
            printf("'E' button was pressed. Exiting.\n");
            GAME_RUNNING = false;
            break;
        case 102:                       /* 'f' key */
            *direction = 0;
            // if hook hasn't been thrown yet
            if (boat->hookPosY == 0) {
                *isFishing        = true;
                *hookDirection    = 1;      // down
            }
            // if hook is already in the water
            else {
                *hookDirection    = -1;     // up
            }
            break;
        case 104:                       /* 'h' key */
            strcpy(MESSAGE, CONTROLS);
            break;
        default:                        /* None or other key was pressed */
            *direction = 0;     /* idle */
            strcpy(MESSAGE, HELP_CONTROLS);
    }
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
