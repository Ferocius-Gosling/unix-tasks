#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

int handle_error(char *message, int lock_fd, int file_fd) {
    printf(message);
    printf("\n");
    if (errno)
        perror(" ");
    close(lock_fd);
    if (file_fd != -1)
        close(file_fd);
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <filename> <sleep_time>\n", argv[0]);
        return 1;
    }

    int current_pid = getpid();

    char *filename = argv[1];
    int sleep_time = atoi(argv[2])  ;
    char lock_filename[strlen(filename) + 5];
    sprintf(lock_filename, "%s.lck", filename);

    int lock_fd = open(lock_filename, O_CREAT | O_EXCL | O_RDWR, 0777); // падает с ошибкой если флаг не существует, нужно ожидать удаления файла.
    while (lock_fd == -1) {
        sleep(0.1);
        lock_fd = open(lock_filename, O_CREAT | O_EXCL | O_RDWR, 0777);
    }

    if (write(lock_fd, &current_pid, sizeof(current_pid)) == -1) { //пишем pid в лок
        return handle_error("Can't write in .lck file", lock_fd, -1);
    }
    //close(lock_fd);

    int file_fd = open(filename, O_RDWR); //открываем файл блокируемый для чтения/записи
    if (file_fd == -1) {
        handle_error("Can't open locked file", lock_fd, file_fd);
        return 1;
    }

    sleep(sleep_time); // притворились что пишем/читаем

    int read_pid;
    //lock_fd = open(lock_filename, O_RDONLY);
    // if (lock_fd == -1) {
    //     return handle_error(".lck file doesn't exist", lock_fd, file_fd);
    // }
    lseek(lock_fd, 0, SEEK_SET);
    if (read(lock_fd, &read_pid, sizeof(read_pid)) == -1) {
        return handle_error("Can't read from .lck file", lock_fd, file_fd);
    }

    if (read_pid == current_pid) {
        close(file_fd);
        close(lock_fd);

        if (unlink(lock_filename) == -1) {
            return handle_error("Can't delete .lck file", lock_fd, file_fd);
        }
    } 
    else {
        return handle_error(".lck file was corrupted", lock_fd, file_fd);
    }

    return 0;
}