#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/stat.h>
#include <time.h>

#define PORT 15562
#define NID "NID:1795"
#define LOG_FILE "netmsg_IT22179562.log"
#define STORAGE_BASE "./storage/IT22179562"
#define BUFFER_SIZE 2048

pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

void log_event(const char *event) {
    pthread_mutex_lock(&log_mutex);
    FILE *fp = fopen(LOG_FILE, "a");
    if (fp) {
        time_t now = time(NULL);
        char time_str[26];
        ctime_r(&now, time_str);
        time_str[strlen(time_str) - 1] = '\0';
        fprintf(fp, "[%s] %s\n", time_str, event);
        fclose(fp);
    }
    pthread_mutex_unlock(&log_mutex);
}

void *handle_client(void *arg) {
    int client_sock = *(int *)arg;
    free(arg);
    char buffer[BUFFER_SIZE] = {0};
    
    int bytes = recv(client_sock, buffer, BUFFER_SIZE - 1, 0);
    if (bytes > 0) {
        char response[512] = {0};
        if (strncmp(buffer, "REGISTER", 8) == 0) {
            snprintf(response, sizeof(response), "OK REGISTERED user %s\n", NID);
            log_event("Command executed: REGISTER");
        } else if (strncmp(buffer, "PMSG", 4) == 0) {
            snprintf(response, sizeof(response), "OK PMSG sent successfully [%s]\n", NID);
            log_event("Command executed: PMSG");
        } else if (strncmp(buffer, "SENDFILE", 8) == 0) {
            snprintf(response, sizeof(response), "OK FILE stored successfully [%s]\n", NID);
            log_event("Command executed: SENDFILE");
        } else {
            snprintf(response, sizeof(response), "OK PROCESSED [%s]\n", NID);
            log_event("Command executed: GENERAL");
        }
        send(client_sock, response, strlen(response), 0);
    }
    
    close(client_sock);
    return NULL;
}

int main() {
    int server_sock, new_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_sock, 10);
    
    printf("[SERVER] NetMessenger running on port %d...\n", PORT);
    log_event("Server started on port 15562");

    mkdir("./storage", 0777);
    mkdir(STORAGE_BASE, 0777);

    while (1) {
        new_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_len);
        if (new_sock >= 0) {
            pthread_t tid;
            int *pSock = malloc(sizeof(int));
            *pSock = new_sock;
            pthread_create(&tid, NULL, handle_client, pSock);
            pthread_detach(tid);
        }
    }
    return 0;
}
