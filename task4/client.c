#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
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

int main(int argc, char *argv[]) {
    int sock_fd;
    socklen_t len;
    struct sockaddr_un address;
    int result;
    char *to_send = NULL;
    char *to_recv = NULL;

    if (argc != 3) {
        printf("Prompt should be like: %s <config_filename> <number_of_reads>\n", argv[0]);
        return 1;
    }

    sigaction(SIGPIPE, &(struct sigaction){sigpipe_handler}, NULL);

    char socket_name[MAX_LINE_LENGTH];
    char *name_buf = malloc(MAX_LINE_LENGTH);
    int num_reads = atoi(argv[2]);

    get_socketname_from_config(argv[1], name_buf);
    sprintf(socket_name, "/tmp/%s", name_buf);

    memset(&address, 0, sizeof(struct sockaddr_un));
    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    address.sun_family = AF_UNIX;

    strcpy(address.sun_path, socket_name);
    len = sizeof(address);

    result = connect(sock_fd, (struct sockaddr *)&address, len);

    if (result == -1) {
        perror("oops: client");
        exit(1);
    }

    for(int i = 0; i < num_reads; i++) {
        to_recv = malloc(READ_MAX + 1);
        to_send = malloc(READ_MAX + 1);
        scanf("%10s", to_send);
        
        write(sock_fd, to_send, strlen(to_send));
        read(sock_fd, to_recv, READ_MAX);
        
        free(to_send);
        free(to_recv);
    }
    close(sock_fd);
    free(name_buf);
    
    return 0;
}