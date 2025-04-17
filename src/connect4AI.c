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

static int evaluate(State *state) {
    const uint64_t bx = state->bitboardX;
    const uint64_t bo = state->bitboardO;
    const uint64_t be = state->bitboardEmpty;
    const uint64_t playable = state->playable;
    uint64_t mask, bitCountEmpty, bitCountX, bitCountO;
    int i = 0;
    uint64_t threatX = 0, threatO = 0;
    uint64_t critical, emptyMask, xMask, oMask;
    uint64_t opportunityX=0, opportunityO=0; 
    int evaluation = 0;
    state->threatX=0;
    state->threatO=0;
    for (; i < 69; i += 1) {
        mask = eligibleMasks[i];
        emptyMask = be & mask;
        xMask = bx & mask;
        oMask = bo & mask;

        bitCountEmpty = __builtin_popcountll(emptyMask);
        bitCountX = __builtin_popcountll(xMask);
        bitCountO = __builtin_popcountll(oMask);

        threatX = (bitCountX == 3) && (bitCountEmpty == 1);
        threatO = (bitCountO == 3) && (bitCountEmpty == 1);
        critical = (emptyMask & playable);
        state->threatX |= threatX * critical;
        state->threatO |= threatO * critical;

        opportunityX |= (bitCountX == 2) && (bitCountEmpty == 2);
        opportunityO |= (bitCountO== 2) && (bitCountEmpty == 2);
    }

    evaluation += 10*__builtin_popcountll(state->threatX) - 10*__builtin_popcountll(state->threatO);
    evaluation += __builtin_popcountll((opportunityX)) - __builtin_popcountll((opportunityO));

    if (__builtin_expect(state->move > 42, 0))
        return 0;
    else
        return evaluation;
}

static int fastEvaluate(State *state) {
    const uint64_t bx = state->bitboardX;
    const uint64_t bo = state->bitboardO;
    const uint64_t be = state->bitboardEmpty;
    const uint64_t playable = state->playable;
    int i = 0;
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

        threatX = (__builtin_popcountll(xMask) == 3) && (__builtin_popcountll(emptyMask) == 1);
        threatO = (__builtin_popcountll(oMask) == 3) && (__builtin_popcountll(emptyMask) == 1);
        critical = (emptyMask & playable);
        state->threatX |= threatX * critical;
        state->threatO |= threatO * critical;

    }

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
        entries[s].absoluteAnalysisDepth=0;
        entries[s].flag=0;
    }
    return cache;
}

void destroyCache(Cache *cache){
    free(cache->entries);
}

static inline bool getCacheEntry(uint64_t hash, CacheEntry **cacheEntry){
    uint32_t bucket = hash % cache.size;
    *cacheEntry = &cache.entries[bucket];
    return ((*cacheEntry)->hash == hash || (*cacheEntry)->absoluteAnalysisDepth==0);
}

static inline void updateCacheEntry(CacheEntry *entry, int evaluation, uint8_t depth, State *state){
    entry->evaluation = evaluation;
    entry->absoluteAnalysisDepth = depth;
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
            scores[i] = evaluate(state);
            *state = prevState;
        }
    }
    sortingNetwork7(moveOrder, (int*)scores, ascending);
}

int minimaxIteration(State *state, uint8_t maxDepth, bool maximizing, int depth, int alpha, int beta){
    int evaluation;
    // if(depth<15)
        evaluation = evaluate(state);
    // else
        // evaluation = fastEvaluate(state);

    int absoluteDepth = state->move+maxDepth-depth;

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
    if(__builtin_expect((state->move==MAX_DEPTH+1) && (maximizing && state->threatX==0) && (!maximizing && state->threatO==0),0)){
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
    int score=0;
    CacheEntry *cacheEntry;
    bool cacheHit = getCacheEntry(state->hash, &cacheEntry);
    char flag = cacheEntry->flag;
    if(cacheHit){
        if((cacheEntry->absoluteAnalysisDepth >= absoluteDepth) || abs(cacheEntry->evaluation)>9000){
            if (flag == 'e') {
                return cacheEntry->evaluation;
            }
            if (flag == 'l') {
                alpha = MAX(alpha, cacheEntry->evaluation);
            }
            if (flag == 'u') {
                beta = MIN(beta, cacheEntry->evaluation);
            }
            if (alpha >= beta) {
                return cacheEntry->evaluation;
            }
        }
    }

    if(maximizing){
        int best = -INFINITY;
        int originalAlpha = alpha;
        bool earlyStop = false;
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
                    earlyStop=true;
                    break;
                }
            }
        }
        best = (best!= -INFINITY) ? best : 0;
        updateCacheEntry(cacheEntry, best, absoluteDepth, state);
        if (earlyStop)
            cacheEntry->flag = 'l';
        else if (best <= originalAlpha)
            cacheEntry->flag = 'u';
        else
            cacheEntry->flag = 'e';
        return best;
    }
    else{
        int best = INFINITY;
        int originalBeta = beta;
        bool earlyStop = false;
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
                    earlyStop=true;
                    break;
                }
            }
        }

        best = (best!= -INFINITY) ? best : 0;
        updateCacheEntry(cacheEntry, best, absoluteDepth, state);
        if (earlyStop)
            cacheEntry->flag = 'u';
        else if (best >= originalBeta)
            cacheEntry->flag = 'l';
        else
            cacheEntry->flag = 'e';
        return best;
    }
}

//minimax depth first search at a given depth. X is maximizing, O is minimizing.
Move minimaxAI(State *state, uint8_t maxDepth, bool maximizing){
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
        return finalMove;
    }
}

int IDS(State *state, uint8_t maxDepth, int milliseconds, bool singlePass){
    clock_t start;
    start = clock();
    char check = stateCheck(state);
    if(check != '.'){
        printf("Final position, winner: %c\n", check);
        return 3;
    }
    Move bestMove;
    bestMove.move=3; bestMove.score=0;
    bool maximizing = state->player == 'X';
    float maxSeconds = (float)milliseconds / 1000;
    double elapsedTime;
    int depth;
    if(singlePass)
        depth = MIN(maxDepth,MAX_DEPTH-state->move);
    else
        depth = 1;
    
    for(; depth<=MIN(maxDepth,MAX_DEPTH-state->move); depth+=1){
        IDSpass = depth;
        printf("Depth: %d\n", depth);
        bestMove = minimaxAI(state, depth, maximizing);
        printf("Best %d with score %d\n", bestMove.move+1, bestMove.score);
        elapsedTime = (double)((clock()-start)) / CLOCKS_PER_SEC;
        printf("{%.2fs}\n",elapsedTime);
        if(elapsedTime>maxSeconds){
            break;
        }
    }
    
    printf("bestmove %d\n", bestMove.move+1);
    return bestMove.move;
}