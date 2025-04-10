#ifndef CONNECT4_H
#define CONNECT4_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define INFINITY 2147483647
#define MAX_INPUT 255
#define MAX_DEPTH 43    //this must be the move at which the board is full
#define DEFAULT_DEPTH 43
#define DEFAULT_TIME 1000

extern uint64_t zobrist[2][7][6];
extern uint64_t eligibleMasks[69];

#define bitboardFromIJ(i,j) ((1ull<<(6-i)) << (8*j))

typedef struct{
    char player;        //X or O
    uint8_t move;
    int lastI;
    int lastJ;
    uint64_t bitboardX, bitboardO, bitboardEmpty;
    uint64_t hash;
    uint64_t threatX, threatO;
    uint64_t playable;
    char columnHeight[7];
}State;

typedef struct{
    int move;
    int score;
}Move;

typedef struct{
    uint64_t hash;
    short evaluation;
    uint8_t absoluteAnalysisDepth;
    char flag;  //s:solved, e:exact, u:upper, l:lower
}CacheEntry;

typedef struct{
    CacheEntry *entries;
    int size;
}Cache;

void print_binary(uint64_t num);

uint64_t random64();

Move minimaxAI(State *state, uint8_t maxDepth, bool maximizing);

int IDS(State *state, uint8_t maxDepth, int milliseconds, bool singlePass);

void statePrint(State *state);

void stateAdd(State *state, char player, int column);

char stateCheck(State *state);

Cache newCache(int size);

void destroyCache(Cache *cache);

extern Cache cache;

#endif // CONNECT4_H