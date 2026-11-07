#include "game.h"

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

typedef struct {
    const Game *game;
    long long begin, end;
    Result result;
    int inspect;
} ThreadData;

void *worker(void *argument) {
    ThreadData *data = argument;
    if (data->inspect) sleep(6);
    simulate_part(data->game, data->begin, data->end, &data->result);
    return NULL;
}

void simulate_parallel(const Game *game, Result *result, int threads, int inspect) {
    if (threads == 1 && !inspect) {
        simulate_sequential(game, result);
        return;
    }
    pthread_t ids[threads];
    ThreadData data[threads];
    for (int i = 0; i < threads; i++) {
        data[i].game = game;
        data[i].begin = (long long)game->experiments * i / threads;
        data[i].end = (long long)game->experiments * (i + 1) / threads;
        data[i].inspect = inspect;
        if (i > 0 && pthread_create(&ids[i], NULL, worker, &data[i]) != 0) {
            error("Не удалось создать потоки. Уменьшите -t.\n");
        }
    }
    if (inspect) {
        printf("PID: %ld. Потоки созданы, пауза 6 секунд.\n", (long)getpid());
        fflush(stdout);
    }
    worker(&data[0]);
    for (int i = 1; i < threads; i++) {
        if (pthread_join(ids[i], NULL) != 0) error("Ошибка ожидания потока.\n");
    }
    result->wins1 = 0;
    result->wins2 = 0;
    result->draws = 0;
    for (int i = 0; i < threads; i++) {
        result->wins1 += data[i].result.wins1;
        result->wins2 += data[i].result.wins2;
        result->draws += data[i].result.draws;
    }
}
