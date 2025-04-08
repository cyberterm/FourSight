#include "connect4.h"
#include <time.h>

int IDSpass;
Cache cache;

typedef struct {
    uint64_t state;
    uint64_t increment;
} pcg32_random_t;

uint32_t pcg32_random_r(pcg32_random_t *rng) {
    uint64_t oldstate = rng->state;
    rng->state = oldstate * 6364136223846793005ULL + rng->increment;
    uint32_t xorshifted = ((oldstate >> 18) ^ oldstate) >> 27;
    uint32_t rot = oldstate >> 59;
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

void xorshift64(uint64_t *state) {
    uint64_t x = *state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    *state = x;
}

//###################################################################### EVALUATION ###############################################################################################

static inline int fastEvaluate(State *state) {
    const uint64_t bx = state->bitboardX;
    const uint64_t bo = state->bitboardO;
    const uint64_t be = state->bitboardEmpty;
    const uint64_t playable = state->playable;
    int i = 0;
    // uint64_t winX = 0, winO = 0;
    uint64_t threatX = 0, threatO = 0;
    uint64_t critical, emptyMask, xMask, oMask;
    int evaluation = 0;
    state->threatX=0;
    state->threatO=0;
    for (; i < 69; i += 1) {
        uint64_t mask = eligibleMasks[i];
        emptyMask = be & mask;
        xMask = bx & mask;
        oMask = bo & mask;

        // winX |= (xMask) == mask;
        // winO |= (oMask) == mask;

        threatX = (__builtin_popcountll(xMask) == 3) && (__builtin_popcountll(emptyMask) == 1);
        threatO = (__builtin_popcountll(oMask) == 3) && (__builtin_popcountll(emptyMask) == 1);
        critical = (emptyMask & playable);
        state->threatX |= threatX * critical;
        state->threatO |= threatO * critical;

        // evaluation += threatX - threatO;

    }

    // evaluation += __builtin_popcountll(state->threatX) - __builtin_popcountll(state->threatO);

    // if(__builtin_expect(winX != 0, 0))
    //     return 10000;
    // else if(__builtin_expect(winO != 0, 0))
    //     return -10000;
    if (__builtin_expect(state->move > 42, 0))
        return 0;
    else
        return evaluation;
}

void updateLegalMoves(State *state, bool *moves){
    uint64_t mask = bitboardFromIJ(0,5);
    for (int i=0; i<7; i++){
        moves[i] = state->bitboardEmpty & mask;
        mask = mask >> 1;
    }
}

//###################################################################### CACHE ##################################################################################################

#define DIM_C 2
#define DIM_COL 7


Cache newCache(int size){
    Cache cache;
    CacheEntry *entries = malloc(size * sizeof(CacheEntry));
    cache.entries = entries;
    cache.size = size;
    for(int s=0; s<size; s++){
        entries[s].analysisDepth=0;
    }
    return cache;
}

void destroyCache(Cache *cache){
    free(cache->entries);
}

static inline bool getCacheEntry(uint64_t hash, CacheEntry **cacheEntry){
    uint32_t bucket = hash % cache.size;
    *cacheEntry = &cache.entries[bucket];
    return ((*cacheEntry)->hash == hash) || ((*cacheEntry)->analysisDepth==0);
}

static inline void updateCacheEntry(CacheEntry *entry, int evaluation, uint8_t analysisDepth, State *state){
    entry->evaluation = evaluation;
    entry->analysisDepth = analysisDepth;
    entry->hash = state->hash;
}


//###################################################################### MINIMAX ################################################################################################

void sortingNetwork7(int *moveOrder, int *evaluations, bool ascending) {
    #define SWAP(a, b) if ((evaluations[a] > evaluations[b]) == ascending) { \
        int tempEval = evaluations[a]; evaluations[a] = evaluations[b]; evaluations[b] = tempEval; \
        int tempMove = moveOrder[a]; moveOrder[a] = moveOrder[b]; moveOrder[b] = tempMove; \
    }

    SWAP(0, 1); SWAP(3, 4); SWAP(2, 5); SWAP(0, 3);
    SWAP(1, 4); SWAP(2, 6); SWAP(0, 2); SWAP(3, 5);
    SWAP(4, 6); SWAP(1, 3); SWAP(2, 4); SWAP(5, 6);
    SWAP(1, 2); SWAP(3, 4);

    #undef SWAP
}



static inline void generateMoveOrder(State *state, int *moveOrder, bool ascending, bool *isLegalMove){
    int scores[7] = {0,0,0,0,0,0,0};
    State prevState;
    for(int i=0; i<7; i++){
        if(isLegalMove[i]){
            prevState = *state;
            stateAdd(state, state->player, i);
            scores[i] = fastEvaluate(state);
            *state = prevState;
        }
    }
    sortingNetwork7(moveOrder, (int*)scores, ascending);
}

int minimaxIteration(State *state, uint8_t maxDepth, bool maximizing, int depth, int alpha, int beta){
    int evaluation = fastEvaluate(state);

    //The order of these is important!
    // if(__builtin_expect(evaluation >= 9000,0)){
    //     if(!maximizing){            //when AI maximizes, evaluation happens during minimizing!
    //         return 10000 - depth;
    //     }
    //     else{
    //         return 10000 + depth;
    //     }
    // }
    // else if(__builtin_expect(evaluation <= -9000,0)){
    //     if(!maximizing){
    //         return -10000 - depth;
    //     }
    //     else{
    //         return -10000 + depth;
    //     }
    // }
    if(__builtin_expect(state->move==MAX_DEPTH,0)){
        return 0;
    }
    else if(maximizing && (state->threatX != 0)){
        return 10000 - (depth+1);
    }
    else if (!maximizing && (state->threatO != 0)){
        return -10000 + (depth+1);
    }
    else if(__builtin_expect(depth>=maxDepth,0))
        return evaluation;

    bool isLegalMove[7];
    updateLegalMoves(state, isLegalMove);
    State prevState;
    
    int moveOrder[7] = {3,4,2,5,1,6,0};
    // int scores = {0,0,0,0,0,0,0};
    int score;
    CacheEntry *cacheEntry;
    bool cacheHit = getCacheEntry(state->hash, &cacheEntry);
    if(cacheHit && (cacheEntry->analysisDepth >= maxDepth)){
        return cacheEntry->evaluation;
    }

    if(maximizing){
        // if((depth < 10))
        //     generateMoveOrder(state, (int*)moveOrder,false,(bool*)isLegalMove);
        int best = -INFINITY;
        for(int i=0; i<7; i++){
            int move = moveOrder[i];
            if(isLegalMove[move]){
                prevState = *state;
                stateAdd(state,'X',move);
                score = minimaxIteration(state, maxDepth, false, depth+1, alpha, beta);
                *state = prevState;
                best = (best > score) ? best : score;
                alpha = (alpha > best) ? alpha : best;
                if (beta <= alpha){
                    break;
                }
            }
        }
        best = (best!= -INFINITY) ? best : 0;
        updateCacheEntry(cacheEntry, best, maxDepth, state);

        return best;
    }
    else{
        // if((depth < 10))
        //     generateMoveOrder(state, (int*)moveOrder,true,(bool*)isLegalMove);
        int best = INFINITY;
        for(int i=0; i<7; i++){
            int move = moveOrder[i];
            if(isLegalMove[move]){
                prevState = *state;
                stateAdd(state,'O',move);
                score = minimaxIteration(state, maxDepth, true, depth+1, alpha, beta);
                *state = prevState;
                best = (best < score) ? best : score;
                beta = (beta < best) ? beta : best;
                if (beta <= alpha){
                    break;
                }
            }
        }

        best = (best!= -INFINITY) ? best : 0;
        updateCacheEntry(cacheEntry, best, maxDepth, state);

        return best;
    }
}

//minimax depth first search at a given depth. X is maximizing, O is minimizing.
Move minimaxAI(State *state, uint8_t maxDepth, bool maximizing){
    clock_t start;
    start = clock();
    char check = stateCheck(state);
    if(check != '.'){
        printf("Final position, winner: %c\n", check);
        Move defaultMove;
        defaultMove.move=3; defaultMove.score=0;
        return defaultMove;
    }
    bool isLegalMove[7];
    updateLegalMoves(state, isLegalMove);
    int alpha = -INFINITY;
    int beta = INFINITY;
    State prevState;
    Move finalMove;
    int score;

    int moveOrder[7] = {3,4,2,5,1,6,0};

    if(maximizing){
        int best = -INFINITY;
        int bestMove=3;
        for(int i=0; i<7; i++){
            int move = moveOrder[i];
            if(isLegalMove[move]){
                prevState = *state;
                stateAdd(state,'X',move);
                check = stateCheck(state);
                if(check == 'X')
                    score = 9999;
                else
                    score = minimaxIteration(state, maxDepth, false, 1, alpha, beta);
                printf("Move %d, Score: %d\n", move+1, score);
                *state = prevState;
                if (score > best){
                    best = score;
                    bestMove = move;
                }
                alpha = (alpha > best) ? alpha : best;
                if (beta <= alpha) break;
            }
        }
        finalMove.move = bestMove;
        finalMove.score = best;
        finalMove.time = (double)(clock() - start) / CLOCKS_PER_SEC;
        printf("(%.2fs)\n",finalMove.time);
        return finalMove;
    }
    else{
        int best = INFINITY;
        int bestMove=3;
        for(int i=0; i<7; i++){
            int move = moveOrder[i];
            if(isLegalMove[move]){
                prevState = *state;
                stateAdd(state,'O',move);
                check = stateCheck(state);
                if(check == 'O')
                    score = -9999;
                else
                    score = minimaxIteration(state, maxDepth, true, 1, alpha, beta);
                printf("Move %d, Score: %d\n", move+1, score);
                *state = prevState;
                if (score < best){
                    best = score;
                    bestMove = move;
                }
                beta = (beta < best) ? beta : best;
                if (beta <= alpha) break;
            }
        }
        finalMove.move = bestMove;
        finalMove.score = best;
        finalMove.time = (double)(clock() - start) / CLOCKS_PER_SEC;
        printf("(%.2fs)\n",finalMove.time);
        return finalMove;
    }
}

int IDS(State *state, uint8_t maxDepth, int milliseconds){
    char check = stateCheck(state);
    if(check != '.'){
        printf("Final position, winner: %c\n", check);
        return 3;
    }
    Move bestMove;
    bestMove.move=3; bestMove.score=0;
    double elapsedTime;
    bool maximizing = state->player == 'X';
    float maxSeconds = (float)milliseconds / 1000;

    for(int depth = 1; depth<=MIN(maxDepth,MAX_DEPTH-state->move); depth+=1){
        IDSpass = depth;
        printf("Depth: %d\n", depth);
        bestMove = minimaxAI(state, depth, maximizing);
        printf("Best %d with score %d\n\n", bestMove.move+1, bestMove.score);
        elapsedTime = bestMove.time;
        if(elapsedTime>maxSeconds){
            break;
        }
    }
    
    printf("bestmove %d\n", bestMove.move+1);
    return bestMove.move;
}