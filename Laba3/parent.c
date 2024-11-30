#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>

#define SHM_NAME "/my_shared_memory"
#define SEM_PARENT_NAME "/parent_semaphore"
#define SEM_CHILD_NAME "/child_semaphore"
#define BUFFER_SIZE 1024

typedef struct {
    char buffer[BUFFER_SIZE];
    size_t size;
} shared_data;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        const char *msg = "Usage: ./parent <filename>\n";
        write(STDERR_FILENO, msg, strlen(msg));
        exit(1);
    }

    shm_unlink(SHM_NAME);
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        const char *err = "Error creating shared memory\n";
        write(STDERR_FILENO, err, strlen(err));
        exit(1);
    }

    ftruncate(shm_fd, sizeof(shared_data));
    shared_data *shared = mmap(NULL, sizeof(shared_data), 
                             PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    sem_unlink(SEM_PARENT_NAME);
    sem_unlink(SEM_CHILD_NAME);
    sem_t *sem_parent = sem_open(SEM_PARENT_NAME, O_CREAT, 0666, 0);
    sem_t *sem_child = sem_open(SEM_CHILD_NAME, O_CREAT, 0666, 0);

    pid_t pid = fork();

    if (pid < 0) {
        const char *err = "Error creating child process\n";
        write(STDERR_FILENO, err, strlen(err));
        exit(1);
    }

    if (pid == 0) {
        execl("./child", "./child", argv[1], NULL);
        const char *err = "execl: No such file or directory\n";
        write(STDERR_FILENO, err, strlen(err));
        exit(1);
    } else {
        char buffer[BUFFER_SIZE];
        const char *msg = "Введите команды (формат: число число число<endline>, для выхода нажмите Ctrl+D):\n";
        write(STDOUT_FILENO, msg, strlen(msg));

        while ((shared->size = read(STDIN_FILENO, buffer, sizeof(buffer))) > 0) {
            memcpy(shared->buffer, buffer, shared->size);
            sem_post(sem_child);
            sem_wait(sem_parent);
        }

        shared->size = 0;
        sem_post(sem_child);

        munmap(shared, sizeof(shared_data));
        close(shm_fd);
        shm_unlink(SHM_NAME);
        sem_close(sem_parent);
        sem_close(sem_child);
        sem_unlink(SEM_PARENT_NAME);
        sem_unlink(SEM_CHILD_NAME);
    }

    return 0;
}
