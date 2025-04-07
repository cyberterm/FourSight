#include <ctype.h>
#include <string.h>
#include "connect4.h"

uint64_t zobrist[2][7][6];
uint64_t eligibleMasks[69];

uint64_t random64() {
    // Combine two 32-bit random numbers into one 64-bit number
    uint64_t num1 = (uint64_t)rand() & 0xFFFFFFFF;
    uint64_t num2 = (uint64_t)rand() & 0xFFFFFFFF;
    return (num1 << 32) | num2;
}

char capitalize(char c) {
    return (c >= 'a' && c <= 'z') ? (c - 32) : c;
}

void initZobrist(){
    for (int piece = 0; piece < 2; piece++) {  // 0 for 'X', 1 for 'O'
        for (int col = 0; col < 7; col++) {
            for (int row = 0; row < 6; row++) {
                zobrist[piece][col][row] = random64();
            }
        }
    }
}

void print_binary(uint64_t num) {
    for (int i = 63; i >= 0; i--) {
        putchar((num & (1ULL << i)) ? '1' : '0');
        if (i % 8 == 0) putchar('\n');
    }
}

void initMasks(){
    int index=0;
    uint64_t mask;
    //horizontal
    mask = (1ull | (1ull<<1) | (1ull<<2) | (1ull<<3));
    for(int j=0; j<6; j++){
        for(int i=0; i<4; i++){
            eligibleMasks[index] = mask;
            index++;
            mask = mask << 1;
        }
        mask = mask << 4;
    }
    //vertical
    mask = (1ull | 1ull<<8 | 1ull<<16 | 1ull<<24);
    for(int j=0; j<3; j++){
        for(int i=0; i<7; i++){
            eligibleMasks[index] = mask;
            index++;
            mask = mask << 1;
        }
        mask = mask << 1;
    }
    // Top-left to Bottom-Right
    mask = (1ull | 1ull<<9 | 1ull<<18 | 1ull<<27 );
    for(int j=0; j<3; j++){
        for(int i=0; i<4; i++){
            eligibleMasks[index] = mask;
            index++;
            mask = mask << 1;
        }
        mask = mask << 4;
    }
    mask = (1ull <<3 | 1ull<<10 | 1ull<<17 | 1ull<<24);
    for(int j=0; j<3; j++){
        for(int i=0; i<4; i++){
            eligibleMasks[index] = mask;
            index++;
            mask = mask << 1;
        }
        mask = mask << 4;
    }
}

void stateInit(State *state){
    state->player = 'X';
    state->move = 1;
    state->hash=0;
    state->bitboardX = 0;
    state->bitboardO = 0;
    state->bitboardEmpty = ~0;
    state->threatX = 0;
    state->threatO = 0;
    state->playable = (1ull | 1ull<<1 | 1ull<<2 | 1ull<<3 | 1ull<<4 | 1ull<<5 | 1ull<<6);
    for(int i=0; i<7; i++){
        state->columnHeight[i] = 0;
    }
}

void statePrint(State *state){
    uint64_t index = bitboardFromIJ(0,5);
    printf("Player: %c\n", state->player);
    for(int i=0; i<6; i++){
        for(int j=0; j<7; j++){
            if((index & state->bitboardX) != 0)
                printf("X");
            else if ((index & state->bitboardO) != 0)
                printf("O");
            else
                printf(".");
            printf("   ");
            index = index >> 1;
        }
        printf("\n");
        index = index >> 1;
    }
    printf("\n");
}

void clear_screen() {
#ifdef _WIN32
    system("cls"); 
#else
    system("clear"); 
#endif
}

void stateAdd(State *state, char player, int column){
    int lastj = state->columnHeight[column]++;
    uint64_t mask = bitboardFromIJ(column, lastj);

    if(player == 'X'){
        state->bitboardX |= mask;
        state->player = 'O';
    }
    else{
        state->bitboardO |= mask;
        state->player = 'X';
    }
    state->bitboardEmpty &= ~mask;  // Clear the bit from the empty board
    state->move++;
    state->hash ^= zobrist[(state->player != 'X') ? 0 : 1][column][lastj];
    state->playable &= ~mask;
    state->playable |= (mask << 8) & 0x0000FFFFFFFFFFFF; //The bit above is now playable except if it's above the 6th row
}

char stateCheck(State *state){
    uint64_t mask;
    for(int i=0; i< 69; i++){
        mask = eligibleMasks[i];
        if((mask & state->bitboardX) == mask)
            return 'X';
        else if ((mask & state->bitboardO) == mask)
            return 'O';
    }
    if(state->move==MAX_DEPTH){
        return '-';
    }
    return '.';
}

bool checkMove(State *state, int move){
    return state->bitboardEmpty & bitboardFromIJ(move,5);   //Works for moves in [1,7], not array indices
}

void twoPlayersGame(State *startingState){
    char c;
    char outcome;
    State game;
    char input[MAX_INPUT];
    START:
    game = *startingState;
    outcome=stateCheck(&game);
    stateInit(&game);
    printf("Starting two player game!\n\n");
    printf("Press numbers 1-7 to play\n");
    statePrint(&game);
    while(outcome=='.'){
        if (fgets(input, MAX_INPUT, stdin) != NULL) {
            // Remove the trailing newline character
            input[strcspn(input, "\n")] = '\0';
            c = input[0];
        } else {
            printf("Error reading input.\n");
            continue;
        }
        if(capitalize(c) == 'R'){
            return;
        }
        else if (capitalize(c)=='Q'){
            printf("Exiting game\n");
            return;
        }
        else if(isdigit(c) && c >= '1' && c <= '7' && checkMove(&game, c-'1')){
            printf("You entered: %s\n", input);
            stateAdd(&game, game.player, c-'1');
            outcome = stateCheck(&game);

            printf("Press numbers 1-7 to play\n");
            statePrint(&game);
        }
        else{
            printf("Invalid input\n");
            continue;
        }
    }
    if(outcome=='-'){
        printf("Draw!\n");
    }
    else{
        printf("%c has won!\n", outcome);
    }
    printf("Play again? (Y/n)");
    c = getchar();
    if(capitalize(c)=='Y')
        goto START;

}

void againstAI(State startingState, int maxDepth, int milliseconds){
    printf("Starting game against AI\n");
    char c;
    char outcome;
    State game;
    char input[MAX_INPUT+1]={0};
    char playerChar;
    START:
    game = startingState;
    outcome=stateCheck(&startingState);
    printf("Pick side (X/O)\n");
    if (fgets(input, MAX_INPUT, stdin) != NULL) {
        input[strcspn(input, "\n")] = '\0';
        playerChar = capitalize(input[0]);
        if(!(playerChar == 'X' || playerChar == 'O')){
            printf("Invalid character pick (X/O)\n");
            goto START;
        }
    } else {
        printf("Error reading input.\n");
        goto START;
    }

    printf("Starting game against AI!\n\n");
    printf("Press numbers 1-7 to play\n");
    statePrint(&game);
    while(outcome=='.'){
        if (game.player == playerChar){
            if (fgets(input, MAX_INPUT, stdin) != NULL) {
                // Remove the trailing newline character
                input[strcspn(input, "\n")] = '\0';
                c = input[0];
            } else {
                printf("Error reading input.\n");
                continue;
            }
            if(capitalize(c) == 'R')
                goto START;
            else if (capitalize(c)=='Q'){
                printf("Exiting game\n");
                return;
            }
            if (isdigit(c) && c >= '1' && c <= '7' && checkMove(&game, c-'1')){
                printf("You entered: %s\n", input);
                stateAdd(&game, game.player, c-'1'); //'0' -> 1
                outcome = stateCheck(&game);

                printf("Press numbers 1-7 to play\n");
                statePrint(&game);
            }
            else{
                printf("Invalid input\n");
                continue;
            }
        }
        else{
            // int bestMove = minimaxAI(&game, adjustedDepth, (playerChar=='X') ? false : true);
            int bestMove = IDS(&game, maxDepth, milliseconds);
            
            stateAdd(&game, game.player, bestMove);
            outcome = stateCheck(&game);

            printf("Press numbers 1-7 to play\n");
            statePrint(&game);
        }
    }
    if(outcome=='-'){
        printf("Draw!\n");
    }
    else{
        printf("%c has won!\n", outcome);
    }
    printf("Play again? (Y/n)");
    c = getchar();
    if(capitalize(c)=='Y')
        goto START;
}

State positionFromString(char s[]){
    State state;
    stateInit(&state);
    int i=0, indexI=0, indexJ=0;
    char c = s[i];
    while(c != '\0'){
        if(isdigit(c)){
            indexI += c - '0';
            if(indexI>=7){
                indexI=0;
                indexJ++;
            }
        }
        else if(c == 'X' || c == 'O'){
            // state.board[indexI][indexJ] = c;
            stateAdd(&state, c, indexI);
            indexI++;
            if(indexI>=7){
                indexI=0;
                indexJ++;
            }
        }

        if(indexI>6 || indexJ>5){
            if(state.move%2 == 0){
                state.player = 'O';
            }
            else{
                state.player = 'X';
            }
            return state;
        }

        i++;
        c = s[i];
    }
    if(state.move%2 == 0){
        state.player = 'O';
    }
    else{
        state.player = 'X';
    }
    return state;
}

void process_command(State *state, char *input) {
    int depth = 0;
    int movetime = 0;
    int hash_size = 0;
    int add_value = 0;
    char position_text[250] = {0};

    char command[20] = {0};
    char param1[50] = {0};
    char param2[50] = {0};

    int parsed = sscanf(input, "%19s %49s %49s", command, param1, param2);

    if (strcmp(command, "go") == 0) {
        if (strcmp(param1, "depth") == 0 && parsed == 3) {
            depth = atoi(param2);
            IDS(state, depth, INFINITY);
        }
        else if (strcmp(param1, "movetime") == 0 && parsed == 3) {
            movetime = atoi(param2);
            IDS(state, MAX_DEPTH, movetime);
        }
        else if (parsed == 1) {
            IDS(state, MAX_DEPTH, INFINITY);
        }
        else {
            printf("Invalid go command\n");
        }
    }
    else if (strcmp(command, "2players") == 0) {
        twoPlayersGame(state);
    }
    else if (strcmp(command, "position") == 0) {
        if (strcmp(param1, "startpos") == 0) {
            stateInit(state);
            statePrint(state);
        }
        else if (parsed >= 2) {
            // Join the remaining parts as position text
            char *text_start = input + strlen(command) + 1;
            strcpy(position_text, text_start);
            *state = positionFromString(position_text);
            statePrint(state);
        }
        else {
            printf("Invalid position command\n");
        }
    }
    else if (strcmp(command, "add") == 0 && parsed == 2) {
        add_value = atoi(param1) - 1;
        stateAdd(state, state->player, add_value);
        statePrint(state);
    }
    else if (strcmp(command, "play") == 0) {
        if (strcmp(param1, "depth") == 0 && parsed == 3) {
            depth = atoi(param2);
            againstAI(*state, depth, INFINITY);
        }
        else if (strcmp(param1, "movetime") == 0 && parsed == 3) {
            movetime = atoi(param2);
            againstAI(*state, MAX_DEPTH, movetime);
        }
        else if (parsed == 1) {
            againstAI(*state, DEFAULT_DEPTH, DEFAULT_TIME);
        }
        else {
            printf("Invalid play command\n");
        }
    }
    else if (strcmp(command, "setoption") == 0) {
        if (strcmp(param1, "hash") == 0 && parsed == 3) {
            hash_size = atoi(param2);
            printf("Set hash size to %d\n", hash_size);
            // Add your specific action for -setoption hash here
        }
        else {
            printf("Invalid setoption command\n");
        }
    }
    else if (strcmp(command, "help") == 0){
        printf(
            "Supported commands:\n"
            "2players           : Play with a friend (No AI)\n"
            "go                 : Search (current position) indefinitely\n"
            "go depth <d>       : Search to depth <d> (e.g., go depth 20)\n"
            "go movetime <s>    : Search for <s> milliseconds (e.g., go movetime 20).\n"
            "\t\t\t(The last depth calculation will continue over this time so you need to account for that)\n"
            "position startpos  : Clear the board\n"
            "position <text>    : Insert a position as a string\n"
            "\t\t\tExample: position 2X4/2O4/7/7/7/7 adds an X and an O to the third column. Numbers count spaces and must be accurate(!).\n"
            "\t\t\tDashes or any other characters are just visual aid \n"
            "add <i>            : Add integer <i> to internal state\n"
            "play               : Start a game with the AI with default settings\n"
            "play depth <d>     : Start a game with the AI and set it to search up to depth <d>\n"
            "play movetime <s>  : Start a game with the AI and set it to search for <s> milliseconds\n"
            // "setoption hash <h> : Set hash table size to <h> MB\n"
            "print              : Print the current state\n"
            "help               : Show this help message\n\n"
            "*During a game enter numbers 1-7 to play a column or enter r to restart / q to quit\n\n"
        );
    }
    else if (strcmp(command, "print") == 0){
        statePrint(state);
    }
    else {
        printf("Unknown command: %s\n", command);
    }
}

int main(){
    clear_screen();
    initZobrist();
    initMasks();
    char input[MAX_INPUT] = {0};

    printf("\nEnter \"help\" for the list of commands\n");

    State state;
    stateInit(&state);
    statePrint(&state);
    // state = positionFromString("1XOXOO1/2XOO2/2XXX2/3O3/7/7");
    // stateAdd(&state, 'X', 6);
    // stateAdd(&state, 'O', 2);

    // twoPlayersGame();
    // againstAI(state, 'X', MAX_DEPTH, 20);
    
    // IDS(&state, MAX_DEPTH, 4*60);

    while (1) {
        if (fgets(input, MAX_INPUT, stdin) != NULL) {
            input[strcspn(input, "\n")] = '\0';
            
            if (strlen(input) == 0) {
                continue;
            }
            
            process_command(&state, input);
        }
    }
    
    return 0;
}