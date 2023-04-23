#include <stdio.h>
#include <syslog.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/param.h>
#include <sys/resource.h>
#include <string.h>
#include <errno.h>

#define MAX_LINE_LENGTH 1024
#define MAX_SUBPROCCESS 32
#define MAX_ARGS 32

pid_t pid_list[MAX_SUBPROCCESS];
int pid_count;
char config_filename[MAX_LINE_LENGTH];
FILE *log_file;

struct config_info {
    int executables_count;
    char path_to_executable[MAX_LINE_LENGTH][MAX_SUBPROCCESS];
    char path_to_stdin[MAX_LINE_LENGTH];
    char path_to_stdout[MAX_LINE_LENGTH];
};

struct task_info {
    int args_count;
    char **args;
};

void check_absolute_path(char *path){
    if (path[0] != '/') {
        printf("This is not absolute path: %s\n", path); // проверили на абсолютность
        exit(1);
    }
}

char* remove_endline_sym(char *line) {
    int last_sym = strlen(line) - 1;
    if (line[last_sym] == '\n')
        line[strlen(line)-1] = '\0';
    return line;
}

char** add_line(char **array, char* buffer, int count){
    array = (char**)realloc(array, (count+1)*sizeof(*array)); //ладно, перевыделили память для массива
    // взяли размер *array и умножили его на count + 1. то есть по факту короче просто ещё 1024 байта добавили из памяти.
    array[count-1] = (char*)malloc(MAX_LINE_LENGTH); //выделили память в массиве для нового слова, запомнили указатель на эту область.
    strcpy(array[count-1], buffer); // скопировали туда строку.
    return array;
}

struct config_info read_config_file(char *config_filename){
    check_absolute_path(config_filename);
    FILE *config_file = fopen(config_filename, "r");
    if (config_file == NULL) {
        perror("Failed to open config file.");
        exit(1);
    }

    char buffer[MAX_LINE_LENGTH];
    char **paths = NULL;
    int count = 0;
    struct config_info conf;

    while(fgets(buffer, MAX_LINE_LENGTH, config_file) != NULL) { //прочитали в буфер строку.
        count++; // прочитали рофлан увеличили индекс
        paths = add_line(paths, buffer, count);
    }

    strcpy(conf.path_to_stdin, remove_endline_sym(paths[count-2]));
    strcpy(conf.path_to_stdout, remove_endline_sym(paths[count-1])); // вроде воркает

    for (int i = 0; i < count - 2; i++) {
        strcpy(conf.path_to_executable[i], remove_endline_sym(paths[i]));
    }
    conf.executables_count = count - 2;
    free(paths);
    fclose(config_file);
    return conf;
}

void check_conf_for_absolute_paths(struct config_info conf) { 
    for (int i = 0; i < conf.executables_count; i++){ 
        check_absolute_path((char*)conf.path_to_executable[i]);
    }
    check_absolute_path(conf.path_to_stdin);
    check_absolute_path(conf.path_to_stdout);
}

void change_directory_on_root() {
    char *new_dir = "/";

    if (chdir(new_dir) != 0) {
        printf("Failed to change directory.\n");
        exit(1);
    }
}

void close_all_fds() {
    struct rlimit fd_limit;
    getrlimit(RLIMIT_NOFILE, &fd_limit);
    for (int fd = 0; fd < fd_limit.rlim_max; fd++)
        close(fd);
}

void set_daemon_mode() {
    if (getppid() != 1) { // если родительский процесс не init.
        // то надо игнорить сигналы ввода-вывода от терминала, или что там нас запустило.
        signal(SIGTTOU, SIG_IGN);
        signal(SIGTTIN, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);

        if (fork() != 0) { // создали клона, чтобы стать независимым от терминала и сделаем его лидером группы
            exit(0); // убили отца
        }
        setsid(); // новый вызов теперь главный. 
    }
}

FILE* open_log() {
    FILE *log = fopen("/tmp/myinit.log", "wa");
    fwrite("Start daemon\n", 1, sizeof("Start daemon"), log);
    fflush(log);
    // if (errno){
    //     printf("Error while writing in log file\n");
    //     fflush(stdout);
    //     exit(1);
    // }
    return log;
}

void write_log(char *message, FILE *log) {
    fwrite(message, 1, strlen(message), log);
    fflush(log);
    // if (errno){
    //     printf("Error while writing in log file\n");
    //     fflush(stdout);
    //     exit(1);
    // }
}

void change_streams(struct config_info conf) {
    freopen(conf.path_to_stdin, "r", stdin);
    freopen(conf.path_to_stdout, "w", stdout);
}

struct task_info parse_args(char str[MAX_LINE_LENGTH]) {
    char buffer[MAX_LINE_LENGTH];
    strcpy(buffer, str);
    struct task_info t_info;
    char **array = NULL; //буферный массив. СУКА. ёкарный бабай.
    char *split = strtok(buffer, " "); // получили первую строку до пробела(название)
    // strcpy(t_info.name, split);
    // split = strtok(NULL, " "); //находим некст аргумент

    int args_count = 0;
    while (split != NULL) // в сплите находится либо нулл, либо аргумент. его размер неизвестен, там указатель.
    {   // надо выделить память для очереднего прикола. 
        // 
        args_count++;
        array = add_line(array, split, args_count);
        split = strtok(NULL, " ");
    }
    
    args_count++;
    array[args_count - 1] = NULL;
    t_info.args_count = args_count;
    t_info.args = array; // вот здесь потенциальная утечка
    // for (int i = 0; i < args_count; i++) {
    //     strcpy(t_info.args[i], array[i]);
    // }
    return t_info;
}

pid_t start_proc(struct task_info t_info, FILE *log, int task_number) {
    char buffer[MAX_LINE_LENGTH];
    pid_t cpid = fork();
    switch(cpid){
        case -1:
            // printf("Fork error: cpid == -1\n");
            write_log("Fork error: cpid == -1\n", log);
            break;
        case 0: //мы родились вот тут надо запустить то чо в конфиге указано
            // в теории надо уметь стартовать таск, затем его перезапускать.
            // умирать разрешается только в сигхупе.
            // чтобы его стартануть надо как-то аргументы распарсить.                 
            //const char *task_args[t_info.args_count + 1] = t_info.args;
            // printf("%d\n", t_info[p].args_count);
            // fflush(stdout);
            // printf("%s\n", t_info[p].args[0]);
            // fflush(stdout);
            // printf("%s\n", t_info[p].args[1]);
            // fflush(stdout);
            // printf("%p\n", t_info[p].args[2]);
            // fflush(stdout);
            execv(t_info.args[0], t_info.args);
            break;
        default:
            sprintf(buffer, "Fork instance %d started and his pid: %d\n", task_number, cpid);
            write_log(buffer, log);
            pid_list[task_number] = cpid;
            pid_count++;
            break;
    }
    return cpid;
}

void run(struct config_info conf, FILE *log) {
    pid_t cpid;
    int p;
    struct task_info t_info[conf.executables_count];
    char buffer[MAX_LINE_LENGTH];

    for (p = 0; p < conf.executables_count; p++){ // по идее всё что ниже надо выделить в функцию, которая бы стартовала проги
        t_info[p] = parse_args(conf.path_to_executable[p]); // получим инфу о таске.
        cpid = start_proc(t_info[p], log, p);
    }

    while(pid_count) { //пока процессы существуют
        cpid = waitpid(-1, NULL, 0); // ждём пока кто-нибудь не умрёт, ну или не закончится.
        for(p = 0; p < conf.executables_count; p++){
            if (pid_list[p] == cpid){
                sprintf(buffer, "Child with number %d and pid: %d finished\n", p, cpid);
                write_log(buffer, log);
                pid_list[p] = 0;
                pid_count--;
                cpid = start_proc(t_info[p], log, p);
            }
        }
    }

    for (p = 0; p < conf.executables_count; p++){
        free(t_info[p].args);
    }
}

void sighup_handler(int sig){
    //signal(SIGHUP, sighup_handler);

    for (int p = 0; p < MAX_SUBPROCCESS; p++){
        if (pid_list[p] != 0)
            kill(pid_list[p], SIGKILL);
    }

    struct config_info conf = read_config_file(config_filename);
    check_conf_for_absolute_paths(conf);
    write_log("Handling hup-signal. Updating config, restarting processes\n", log_file);

    run(conf, log_file);
}

int main(int argc, char *argv[]){
    if (argc != 2) {
        printf("Usage: %s <config_file>\n", argv[0]);
        return 1;
    } // получили конфиг 
    strcpy(config_filename, argv[1]);

    change_directory_on_root(); // поменялись на рута
    close_all_fds(); // закрыли все дескрипторы
    set_daemon_mode(); // стали демоном
    struct sigaction act;
    sigemptyset(&act.sa_mask);
    act.sa_handler = sighup_handler;
    act.sa_flags = SA_NODEFER;
    sigaction(SIGHUP, &act, NULL);
    // куда-то сюда надо будет вернуться после сигхупа

    struct config_info conf = read_config_file(config_filename); // прочитали файл
    //printf("%s\n", conf.path_to_stdin);
    check_conf_for_absolute_paths(conf); // проверили пути на абсолютность
    change_streams(conf);   // перенаправили потоки
    log_file = open_log(); // Открыли лог

    //struct task_info t = parse_args(conf.path_to_executable[0]);

    run(conf, log_file);

    //printf("Running is over\n");
    //sleep(10);
    return 0;
}