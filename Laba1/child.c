#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

int write_to_buffer(char *buffer, size_t buffer_size, const char *str) {
    size_t len = strlen(str);
    if (len > buffer_size) {
        len = buffer_size;
    }
    memcpy(buffer, str, len);
    return len;
}

void itoa(int num, char *str) {
    int i = 0, isNegative = 0;
    if (num < 0) {
        isNegative = 1;
        num = -num;
    }
    do {
        str[i++] = (num % 10) + '0';
        num /= 10;
    } while (num > 0);
    if (isNegative) {
        str[i++] = '-';
    }
    str[i] = '\0';
    for (int j = 0, k = i - 1; j < k; j++, k--) {
        char temp = str[j];
        str[j] = str[k];
        str[k] = temp;
    }
}

int format_result(int numerator, int denominator, char *result, size_t size) {
    char num_str[32], denom_str[32], res_str[32];
    itoa(numerator, num_str);
    itoa(denominator, denom_str);
    float div_result = (float)numerator / denominator;
    int int_part = (int)div_result;
    itoa(int_part, res_str);
    int offset = 0;
    offset += write_to_buffer(result + offset, size - offset, num_str);
    offset += write_to_buffer(result + offset, size - offset, " / ");
    offset += write_to_buffer(result + offset, size - offset, denom_str);
    offset += write_to_buffer(result + offset, size - offset, " = ");
    offset += write_to_buffer(result + offset, size - offset, res_str);
    offset += write_to_buffer(result + offset, size - offset, "\n");
    return offset;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        const char *msg = "Usage: ./child <filename>\n";
        write(STDERR_FILENO, msg, strlen(msg));
        exit(1);
    }

    char line[1024];
    int numbers[100];
    int count;
    int fd = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (fd == -1) {
        const char *err = "Error opening file\n";
        write(STDERR_FILENO, err, strlen(err));
        exit(1);
    }

    ssize_t bytesRead;
    while ((bytesRead = read(STDIN_FILENO, line, sizeof(line) - 1)) > 0) {
        line[bytesRead] = '\0';
        count = 0;

        char *token = strtok(line, " ");
        while (token != NULL && count < 100) {
            numbers[count++] = atoi(token);
            token = strtok(NULL, " ");
        }

        if (count < 2) {
            const char *msg = "Недостаточно чисел в строке\n";
            write(STDERR_FILENO, msg, strlen(msg));
            continue;
        }

        int numerator = numbers[0];
        for (int i = 1; i < count; i++) {
            if (numbers[i] == 0) {
                const char *err = "Ошибка: деление на 0\n";
                write(STDERR_FILENO, err, strlen(err));
                write(fd, err, strlen(err));
                close(fd);
                exit(1);
            }

            char result[128];
            int length = format_result(numerator, numbers[i], result, sizeof(result));
            write(fd, result, length);
            write(STDOUT_FILENO, result, length);
        }
    }

    close(fd);
    return 0;
}