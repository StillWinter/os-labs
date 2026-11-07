#include "game.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void error(const char *text) {
    fprintf(stderr, "%s", text);
    exit(1);
}

int read_number(const char *text) {
    char *end;
    errno = 0;
    long value = strtol(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || value < 0 || value > INT_MAX) {
        error("Ожидалось неотрицательное целое число типа int.\n");
    }
    return (int)value;
}

double current_time_ms(void) {
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    return time.tv_sec * 1000.0 + time.tv_nsec / 1000000.0;
}

int main(int argc, char *argv[]) {
    if ((argc != 13 && argc != 14) || strcmp(argv[1], "-k") != 0 ||
        strcmp(argv[3], "-r") != 0 || strcmp(argv[5], "-a") != 0 ||
        strcmp(argv[7], "-b") != 0 || strcmp(argv[9], "-n") != 0 ||
        strcmp(argv[11], "-t") != 0 || (argc == 14 && strcmp(argv[13], "-s") != 0)) {
        error("Использование: ./main -k K -r ТУР -a ОЧКИ1 -b ОЧКИ2 "
              "-n ОПЫТЫ -t ПОТОКИ [-s]\n");
    }
    Game game;
    game.k = read_number(argv[2]);
    game.round = read_number(argv[4]);
    game.score1 = read_number(argv[6]);
    game.score2 = read_number(argv[8]);
    game.experiments = read_number(argv[10]);
    int limit = read_number(argv[12]);
    int inspect = argc == 14;
    if (game.k == 0 || game.k == INT_MAX || game.round == 0 || game.round > game.k + 1 ||
        game.experiments == 0 || limit == 0 || limit > 1024) {
        error("Нужно: K от 1 до INT_MAX-1, тур от 1 до K+1, "
              "опыты > 0, потоки от 1 до 1024.\n");
    }
    int threads = limit;
    if (threads > game.experiments) threads = game.experiments;

    Result sequential, parallel;
    double start = current_time_ms();
    simulate_sequential(&game, &sequential);
    double sequential_ms = current_time_ms() - start;
    start = current_time_ms();
    simulate_parallel(&game, &parallel, threads, inspect);
    double parallel_ms = current_time_ms() - start;
    int ok = results_equal(&sequential, &parallel);

    printf("Всего туров: %d\nТекущий тур: %d\nОсталось туров: %d\n",
           game.k, game.round, game.k - game.round + 1);
    printf("Очки игроков: %d %d\nЭкспериментов: %d\n",
           game.score1, game.score2, game.experiments);
    printf("Лимит потоков: %d\nИспользовано потоков: %d\n", limit, threads);
    printf("Побед первого: %lld\nПобед второго: %lld\nНичьих: %lld\n",
           parallel.wins1, parallel.wins2, parallel.draws);
    printf("Шанс победы первого, %%: %.4f\n", 100.0 * parallel.wins1 / game.experiments);
    printf("Шанс победы второго, %%: %.4f\n", 100.0 * parallel.wins2 / game.experiments);
    printf("Шанс ничьей, %%: %.4f\n", 100.0 * parallel.draws / game.experiments);
    if (!inspect) {
        double speedup = parallel_ms > 0 ? sequential_ms / parallel_ms : 0;
        printf("Последовательно, мс: %.3f\nПараллельно, мс: %.3f\n",
               sequential_ms, parallel_ms);
        printf("Ускорение: %.3f\nЭффективность, %%: %.3f\n",
               speedup, speedup / threads * 100);
    }
    printf("Проверка: %s\n", ok ? "OK" : "ОШИБКА");
    return ok ? 0 : 1;
}
