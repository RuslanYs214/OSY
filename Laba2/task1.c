#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <stdlib.h>


sem_t thread_sem;
#define DECK_SIZE 52
pthread_mutex_t success_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    int rounds;
    int successes;
} ThreadData;

void shuffle(int *deck) {
    for (int i = DECK_SIZE - 1; i > 0; i--) {
        int j = rand() & (i + 1);
        int tmp = deck[i];
        deck[i] = deck[j];
        deck[j] = tmp;
    }
}

void* monte_carlo(void* arg) {
    ThreadData *data = (ThreadData*)arg;
    int local_successes = 0;

    for (int i = 0; i < data->rounds; i++) {
        int deck[DECK_SIZE];
        for (int j = 0; j < DECK_SIZE; j++) {
            deck[j] = j / 4;
        }

        shuffle(deck);
        if (deck[0] == deck[1]) {
            local_successes++;
        }
    }

    if (pthread_mutex_lock(&success_mutex) != 0) {
        char msg[] = "Error lock mutex\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        pthread_exit((void*)1);
    }

    data->successes += local_successes;

    if (pthread_mutex_unlock(&success_mutex) != 0) {
        char msg[] = "Error unlock mutex\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        pthread_exit((void *)1);
    }

    if (sem_post(&thread_sem) != 0) {
        char msg[] = "Error semaphore post\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
    }

    pthread_exit(NULL);
}

int my_atoi(const char *str, int *out) {
    int result = 0;
    int i = 0;
    if (str[i] == '\0' || str[i] == '-') {
        return -1;
    }

    for (; str[i] != '\0'; i++) {
        if (str[i] < '0' || str[i] > '9') {
            return -1;
        }
        result = result * 10 + (str[i] - '0');
    }
    if (result == 0) {
        return -1;
    }

    *out = result;
    return 0;
}

int probability_convert_str(char *str, double probability) {
    int int_part = (int)probability;
    int frac_part = (int)((probability - int_part) * 10000);

    int i = 0;
    str[i++] = '0' + int_part;
    str[i++] = '.';
    for (int j = 1000; j > 0; j /= 10) {
        str[i++] = '0' + (frac_part / j) % 10;
    }

    str[i++] = '\n';
    return i;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        char msg[] = "Usage: ./main <rounds> <max_threads>\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return 1;
    }

    int rounds;
    int max_threads;
    if (my_atoi(argv[1], &rounds) != 0 || my_atoi(argv[2], &max_threads) != 0) {
        char msg[] = "Invalid input line arguments\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return 1;
    }

    if (sem_init(&thread_sem, 0, max_threads) != 0) {
        char msg[] = "Error init semaphore\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return 1;
    }

    pthread_t threads[max_threads];
    ThreadData data = {rounds / max_threads, 0};

    for (int i = 0; i < max_threads; i++) {
        if (sem_wait(&thread_sem) != 0) {
            char msg[] = "Error semaphore wait\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            return 1;
        }

        if (pthread_create(&threads[i], NULL, monte_carlo, &data) != 0) {
            char msg[] = "Error thread creation\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            return 1;
        }
    }

    for (int i = 0; i < max_threads; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            char msg[] = "Error waiting thread\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            return 1;
        }
    }

    double probability = (double)data.successes / rounds;
    char buf[4096];
    int len = probability_convert_str(buf, probability);
    write(STDOUT_FILENO, buf, len);

    if (sem_destroy(&thread_sem) != 0) {
        char msg[] = "Error destroy semaphore";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return 1;
    }

    return 0;
}
