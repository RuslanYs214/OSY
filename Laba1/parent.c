#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        const char *msg = "Usage: ./parent <filename>\n";
        write(STDERR_FILENO, msg, strlen(msg));
        exit(1);
    }

    int pipe1[2], pipe2[2];
    char buffer[1024];

    if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        const char *err = "Error creating pipe\n";
        write(STDERR_FILENO, err, strlen(err));
        exit(1);
    }

    pid_t pid = fork();

    if (pid < 0) {
        const char *err = "Error creating child process\n";
        write(STDERR_FILENO, err, strlen(err));
        exit(1);
    }

    if (pid == 0) { 
        close(pipe1[1]);
        close(pipe2[0]);

        dup2(pipe1[0], STDIN_FILENO);
        close(pipe1[0]);

        dup2(pipe2[1], STDOUT_FILENO);
        close(pipe2[1]);

        execl("./child", "./child", argv[1], NULL);
        const char *err = "execl: No such file or directory\n";
        write(STDERR_FILENO, err, strlen(err));
        exit(1);
    } else {
        close(pipe1[0]);
        close(pipe2[1]);

        const char *msg = "Введите команды (формат: число число число<endline>, для выхода нажмите Ctrl+D):\n";
        write(STDOUT_FILENO, msg, strlen(msg));
        while ((read(STDIN_FILENO, buffer, sizeof(buffer))) > 0) {
            write(pipe1[1], buffer, strlen(buffer));
        }
        close(pipe1[1]);

        ssize_t bytesRead;
        while ((bytesRead = read(pipe2[0], buffer, sizeof(buffer))) > 0) {
            write(STDOUT_FILENO, buffer, bytesRead);
        }
        close(pipe2[0]);
    }

    return 0;
}