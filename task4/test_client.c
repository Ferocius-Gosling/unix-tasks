#include <sys/socket.h>
#include <sys/times.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>

#define MAX_LINE_LENGTH 1024
#define SERV_PORT 123
#define READ_MAX 10

void sigpipe_handler(int unused)
{
    printf("handling sigpipe\n");
}

char* get_socketname_from_config(char config_name[], char* sock_name) {
    FILE *config_file = fopen(config_name, "r");
    if (config_file == NULL) {
        perror("Failed to open config file.");
        exit(1);
    }

    char buffer[MAX_LINE_LENGTH];
    fgets(buffer, MAX_LINE_LENGTH, config_file);
    strcpy(sock_name, buffer);
    return sock_name;
}

FILE* open_log() {
    FILE *log = fopen("/tmp/client.log", "wa");
    // fwrite("Start server\n", 1, sizeof("Start server"), log);
    // fflush(log);
    return log;
}

void write_log(char *message, FILE *log) {
    fwrite(message, 1, strlen(message), log);
    fflush(log);
}

int main(int argc, char *argv[]) {
    int sock_fd;
    socklen_t len;
    struct sockaddr_un address;
    int result;
    //int is_running = 1;
    char *to_send = NULL;
    char *to_recv = NULL;
    char *from_urandom = NULL;

    if (argc != 5) {
        printf("Prompt should be like: %s <config_filename> <number_of_reads> <client_id> <sleep_time>\n", argv[0]);
        return 1;
    }

    sigaction(SIGPIPE, &(struct sigaction){sigpipe_handler}, NULL);

    char socket_name[MAX_LINE_LENGTH];
    char *name_buf = malloc(MAX_LINE_LENGTH);
    int num_reads = atoi(argv[2]);
    int id = atoi(argv[3]);
    float sleep_time = atof(argv[4]) * 1000000; 
    //printf("sleep time %f\n", sleep_time);

    get_socketname_from_config(argv[1], name_buf);
    sprintf(socket_name, "/tmp/%s", name_buf);

    memset(&address, 0, sizeof(struct sockaddr_un));
    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    address.sun_family = AF_UNIX;

    strcpy(address.sun_path, socket_name);
    len = sizeof(address);

    //FILE *log = open_log();
    FILE *file = fopen("/dev/urandom", "r");
    result = connect(sock_fd, (struct sockaddr *)&address, len);

    if (result == -1) {
        perror("oops: client");
        exit(1);
    }
    int seed;
    fread(&seed, sizeof(int), 1, file);
    srand(seed);
    struct tms begin, end;
    //times(&begin);
    time_t t0 = time(0);

    for(int i = 0; i < num_reads; i++) {

        to_recv = malloc(READ_MAX + 1);
        to_send = malloc(READ_MAX + 1);
        scanf("%10s", to_send);
        int w = write(sock_fd, to_send, strlen(to_send));
        int r = read(sock_fd, to_recv, READ_MAX);

        int n = (rand() % 255) + 1;
        from_urandom = malloc(n);
        fread(from_urandom, sizeof(char), n, file);
        usleep(sleep_time);

        free(from_urandom);
        free(to_send);
        free(to_recv);
    }
    //times(&end);
    time_t t1 = time(0);

    double time_in_seconds = difftime(t1, t0);
    //int times_in_seconds = (end.tms_utime - begin.tms_utime);
    //char *log_msg = malloc(MAX_LINE_LENGTH);
    printf(/*log_msg, */"%f\n", time_in_seconds);
    //printf(/*log_msg, */"%d\n", times_in_seconds);
    //write_log(log_msg, log);
    close(sock_fd);
    //free(log_msg);
    free(name_buf);
    
    return 0;
}