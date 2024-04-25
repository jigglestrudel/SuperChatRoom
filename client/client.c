#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <pthread.h>


#define MAX_MESSAGE_LENGTH 255
#define MESSAGE_INPUT_ROW 30

typedef struct {
    int sockfd;
    int* running;
} conn_info;

void move_cursor (int y, int x) {
    printf("\x1B[%d;%dH", y, x);
}

int fake_semaphore = 0;


sem_t terminal_writing;
sem_t awaiting_server_ready;

void* incoming_message_handling(void* connection_info) {
    int sockfd = ((conn_info*)connection_info)->sockfd;
    int* running = ((conn_info*)connection_info)->running;
    char recvBuff[2000];
    int n;
    int line_count = 0;

    
    while (*running == 1 && (n = read(sockfd, recvBuff, sizeof(recvBuff)-1)) > 0) {
        recvBuff[n] = 0;

        if (strcmp(recvBuff, "READY") == 0) {
            fake_semaphore = 1;
            continue;
        }

        //sem_wait(&terminal_writing);

/*
        if (line_count == 0)
        {
            move_cursor(line_count, 0);
            // clear the screen 
            for (int i = 0; i < MESSAGE_INPUT_ROW; i++) {
                printf("                                                                                             \n");
            }
        }*/

        // move cursor to the desired row
        move_cursor(line_count, 0);

        // write out the message
        puts(recvBuff);

        // count another line
        line_count++;

        move_cursor(MESSAGE_INPUT_ROW, 0);
        //sem_post(&terminal_writing);
    } 
    pthread_exit(NULL);

}

void* outcoming_message_handling(void* connection_info) {
    int sockfd = ((conn_info*)connection_info)->sockfd;
    int* running = ((conn_info*)connection_info)->running;
    char message[MAX_MESSAGE_LENGTH + 1];
    int length = 0;
    int c, x, y;

    while (*running)
    {
        length = 0;
        while ((c = getchar()) != '\n')
        {
            message[length] = c;
            length++;
        }

        //message[length] = '\0';

        if (length == 0) continue;

        if (length > 0 && message[0] == '/'){
            if(strcmp(message, "/disconnect") == 0)
            {
                puts("Disconnecting from the server");
                write(sockfd, "DISCONNECT", strlen("DISCONNECT"));
                *running = 0;
                break;
            }   
        }

        write(sockfd, "MSG", strlen("MSG"));

        while(fake_semaphore == 0);
                    
        write(sockfd, message, length);

        fake_semaphore = 0;

        move_cursor(MESSAGE_INPUT_ROW-1, 0);
        printf("                                                         ");                                                                                              
        move_cursor(MESSAGE_INPUT_ROW, 0);
        printf("                                                         "); 
        move_cursor(MESSAGE_INPUT_ROW, 0);
    }
    close(sockfd);
    pthread_exit(NULL);
}



int main(int argc, char *argv[]) {
    int sockfd = 0, n = 0;
    int* running = (int*)malloc(sizeof(int));
    char recvBuff[2000];
    struct sockaddr_in serv_addr; 

    memset(recvBuff, '0',sizeof(recvBuff));
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Error : Could not create socket \n");
        return 1;
    } 

    memset(&serv_addr, '0', sizeof(serv_addr)); 

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(5000); 

    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) {
        printf("\n inet_pton error occured\n");
        return 1;
    } 

    if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
       printf("\n Error : Connect Failed \n");
       return 1;
    } 

    ////////
    // reading server messages
    while ((n = read(sockfd, recvBuff, sizeof(recvBuff)-1)) > 0) {
        recvBuff[n] = '\0';
        if (strcmp(recvBuff, "ASK_NAME") == 0){
            // ask name message
            printf("Nickname:");
            char nickname[100];
            scanf("%s", nickname);
            write(sockfd, nickname, strlen(nickname));
        }
        else if (strcmp(recvBuff, "NAME_GOOD") == 0){
            // name accepted
            printf("Name accepted!\n");
            system("clear");
            break;
        }
    }

    if (n <= 0) { return 1;}
    // create threads responsible for writing and reading from the server
   
    sem_init(&terminal_writing, 0, 1);
    sem_init(&awaiting_server_ready, 0, 0);
    conn_info* connection_info = (conn_info*)malloc(sizeof(conn_info));
    *running = 1;
    connection_info->sockfd = sockfd;
    connection_info->running = running;
    pthread_t input_thread, output_thread;
    pthread_create(&input_thread, NULL, incoming_message_handling, (void*)connection_info);
    pthread_create(&output_thread, NULL, outcoming_message_handling, (void*)connection_info);
    
    pthread_join(input_thread, NULL);
    pthread_join(output_thread, NULL);    

    return 0;
}
