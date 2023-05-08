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
    //char* p_buffer;
    strcpy(sock_name, buffer);
    return sock_name;
}

int main(int argc, char *argv[]) {
    int sock_fd;
    socklen_t len;
    struct sockaddr_un address;
    int result;
    //int is_running = 1;
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
        //printf("before reading\n");
        scanf("%10s", to_send);
        //printf("after reading %s\n", to_send);
        //printf("sock_fd %d\n", sock_fd);

        // int error = 0;
        // socklen_t len = sizeof (error);
        // int retval = getsockopt (sock_fd, SOL_SOCKET, SO_ERROR, &error, &len);
        // if (retval != 0) {
        //     /* there was a problem getting the error code */
        //     printf("error getting socket error code: %s\n", strerror(retval));
        //     return 1;
        // }

        // if (error != 0) {
        //     /* socket has a non zero error status */
        //     printf("socket error: %s\n", strerror(error));
        // }
        //printf("well, this is shit %d %d\n ", retval, error);
        int w = write(sock_fd, to_send, strlen(to_send));
        //printf("i write to server %d bytes\n", w);

        // int q1 = read(sock_fd, to_recv, READ_MAX);
        // printf("i read from server %d bytes\n", q1);

        // int q2 = write(sock_fd, "1234", 4);
        // printf("i write to server 1234 and %d bytes\n", q2);

        //perror("ERROR");
        //printf("%d write\n", w);
        int r = read(sock_fd, to_recv, READ_MAX);
        //printf("%d read\n", r);
        //printf("char from server = %s\n", to_recv);
        free(to_send);
        free(to_recv);
    }
    //printf("i quit cycle\n");
    close(sock_fd);
    free(name_buf);
    //free(to_send);
    
    return 0;
}