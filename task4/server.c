#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h> 
#include <errno.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros 

#define MAX_LINE_LENGTH 1024
#define MAX_BACKLOG 128
#define READ_MAX 10

int state = 0;

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

int create_socket() {
    int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("Can't open socket\n");
        exit(1);
    }

    return sock_fd;
}

struct sockaddr_un create_addr(char *sock_name) {
    struct sockaddr_un addr;
    
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, sock_name, sizeof(addr.sun_path) - 1);

    return addr;
}

FILE* open_log() {
    FILE *log = fopen("/tmp/server.log", "wa");
    fwrite("Start server\n", 1, sizeof("Start server"), log);
    fflush(log);
    return log;
}

void write_log(char *message, FILE *log) {
    fwrite(message, 1, strlen(message), log);
    fflush(log);
}

int main(int argc, char *argv[]) {
    //сначала надо получить название сокета из конфига.
    if (argc != 2) {
        printf("Prompt should be like: %s <config_name>\n", argv[0]);
        return 1;
    }

    socklen_t len;
    ssize_t num_bytes;
    char socket_name[MAX_LINE_LENGTH];
    char *name_buf = malloc(MAX_LINE_LENGTH);
    int option = 1;
    int max_sd, sd, activity, new_socket, valread;
    int *client_sockets = malloc(MAX_LINE_LENGTH * sizeof(int)); // типа массив клиентских сокетов.
    fd_set readfds; // а это для селекта

    FILE *log = open_log();

    get_socketname_from_config(argv[1], name_buf);
    sprintf(socket_name, "/tmp/%s", name_buf);
    unlink(socket_name);
    write_log("Get socket name succesfully\n", log);
    // теперь откроем сокет
    struct sockaddr_un serv_addr, client_addr;
    if (strlen(socket_name) > sizeof(serv_addr.sun_path) - 1) {
        printf("Server socket path too long: %s", socket_name);
        return 1;
    }

    int master_socket = create_socket();

    //тут вроде как разрешаем сокету иметь много соединений.
    if(setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&option, sizeof(option)) < 0 )
    {  
        perror("setsockopt");  
        exit(EXIT_FAILURE);  
    } 

    serv_addr = create_addr(socket_name);

    // теперь чё-то типа ээ. прикрутим к сокету адрес. забиндим.
    if (bind(master_socket, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr_un)) == -1) {
        perror("Server initializing error\n");
        return 1;
    }
    write_log("Bind socket succesfully\n", log);
    // слушаем сокет и ставим макс число в очереди.
    listen(master_socket, MAX_BACKLOG);

    printf("Server initialized\n");

    while (1) {
        int num;

        FD_ZERO(&readfds); //почистили сет
        FD_SET(master_socket, &readfds); //добавили туда мастера
        max_sd = master_socket; // максимальный номер дескриптора. нужен для селекта.

        for (int i = 0; i < MAX_LINE_LENGTH; i++){
            sd = client_sockets[i]; // чекаю сокет
            if (sd > 0)
                FD_SET(sd, &readfds); // если норм, то добавлю туды.
            if (sd > max_sd)
                max_sd = sd; //надо для селекта. 
        }

        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if ((activity < 0) && (errno != EINTR)){
            printf("select get error\n");
        }
        
        // если чото происходит на мастер сокете
        // то это входящее подключение по любому.
        if (FD_ISSET(master_socket, &readfds)) {
            if ((new_socket = accept(master_socket, NULL, NULL)) < 0) {
                perror("can't accept socket");
                exit(EXIT_FAILURE);
            }

            printf("accept socket %d\n", new_socket);

            for (int i = 0; i < MAX_LINE_LENGTH; i++){
                if (client_sockets[i] == 0) { // запомнили сокет, который пришёл к нам
                    client_sockets[i] = new_socket;
                    char *tmp_buf = malloc(64);
                    long sbrk_val = (long) sbrk(0);
                    sprintf(tmp_buf, "Accept socket %d. Current value of sbrk() is %d\n", new_socket, sbrk_val);
                    write_log(tmp_buf, log);
                    free(tmp_buf);
                    break;
                }
            }
        }

        for(int i = 0; i < MAX_LINE_LENGTH; i++) {
            sd = client_sockets[i];
            char *num_from_client = malloc(READ_MAX);// два буфера
            char *num_to_client = malloc(READ_MAX);
            int buf;

            if (FD_ISSET(sd, &readfds)) {
                if (read(sd, num_from_client, READ_MAX) == 0) { //если из чела ничего не читается, но активность была
                    // кто-то дисконнектнулся
                    printf("socked %d disconnected\n", sd);
                    close(sd);
                    client_sockets[i] = 0;
                } else { // а тут если читается.
                    write_log("Get number from client: ", log);
                    write_log(num_from_client, log);
                    write_log("\n", log);
                    num = atoi(num_from_client);
                    
                    state += num;
                    sprintf(num_to_client, "%d", state);
                    write_log("Sending state to client: ", log);
                    write_log(num_to_client, log);
                    write_log("\n", log);
                    write(sd, num_to_client, READ_MAX);
                }
            }
            free(num_from_client);
            free(num_to_client);
        }
    }
    
    close(master_socket);
    free(name_buf);
    return 0;
}