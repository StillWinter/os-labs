#include "game.h"

#include <stdlib.h>

int throw_die(unsigned int *seed) {
    int value;
    int limit = RAND_MAX - RAND_MAX % 6;
    do {
        value = rand_r(seed);
    } while (value >= limit);
    return value % 6 + 1;
}

void simulate_part(const Game *game, long long begin, long long end, Result *result) {
    Result count = {0, 0, 0};
    for (long long i = begin; i < end; i++) {
        unsigned int seed = ((unsigned int)i + 1) * 2654435761u;
        long long score1 = game->score1;
        long long score2 = game->score2;
        for (int round = game->round; round <= game->k; round++) {
            score1 += throw_die(&seed) + throw_die(&seed);
            score2 += throw_die(&seed) + throw_die(&seed);
        }
        if (score1 > score2) count.wins1++;
        else if (score2 > score1) count.wins2++;
        else count.draws++;
    }
    *result = count;
}

void simulate_sequential(const Game *game, Result *result) {
    simulate_part(game, 0, game->experiments, result);
}

int results_equal(const Result *a, const Result *b) {
    return a->wins1 == b->wins1 && a->wins2 == b->wins2 && a->draws == b->draws;
}
