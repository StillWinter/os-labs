#ifndef GAME_H
#define GAME_H

typedef struct {
    int k;
    int round;
    int score1, score2;
    int experiments;
} Game;

typedef struct {
    long long wins1, wins2, draws;
} Result;

void error(const char *text);
void simulate_part(const Game *game, long long begin, long long end, Result *result);
void simulate_sequential(const Game *game, Result *result);
void simulate_parallel(const Game *game, Result *result, int threads, int inspect);
int results_equal(const Result *a, const Result *b);

#endif
