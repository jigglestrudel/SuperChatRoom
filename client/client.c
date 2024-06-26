#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <pthread.h>


#define MAX_MESSAGE_LENGTH 255
#define MESSAGE_INPUT_ROW 15
#define OUR_ADDRESS "172.20.10.2"

typedef struct {
    int sockfd;
    int* running;
    int* fake_semaphore;
} conn_info;

void move_cursor (int y, int x) {
    char str[10];
    snprintf(str, 10, "\x1B[%d;%dH", y, x);
    puts(str);
}

void* incoming_message_handling(void* connection_info) {
    int sockfd = ((conn_info*)connection_info)->sockfd;
    int* running = ((conn_info*)connection_info)->running;
    int* fake_semaphore = ((conn_info*)connection_info)->fake_semaphore;
    char recvBuff[2000];
    int n;
    int line_count = 1;
    
    while (*running == 1 && (n = read(sockfd, recvBuff, sizeof(recvBuff)-1)) > 0) {
        recvBuff[n] = 0;

        if (strcmp(recvBuff, "READY") == 0) {
            // server is ready for our message
            *fake_semaphore = 1;
            continue;
        }

        if (recvBuff[0] == 'M' && recvBuff[1] == 'S' && recvBuff[2] == 'G') {
            puts("\033[s");
            // move cursor to the desired row
            move_cursor(line_count, 0);

            puts(recvBuff+3);
            line_count++;

            // restoting the last position
            puts("\033[u");
            continue;
        }

        puts("\033[s");
        // move cursor to the desired row
        move_cursor(line_count, 0);

        puts(recvBuff);
        line_count++;

        // restoting the last position
        puts("\033[u");
        
    } 
    pthread_exit(NULL);

}

void* outcoming_message_handling(void* connection_info) {
    int sockfd = ((conn_info*)connection_info)->sockfd;
    int* running = ((conn_info*)connection_info)->running;
    int* fake_semaphore = ((conn_info*)connection_info)->fake_semaphore;
    char message[MAX_MESSAGE_LENGTH + 1];
    int length = 0;
    int c, x, y;

    move_cursor(MESSAGE_INPUT_ROW, 0);

    while (*running)
    {
        length = 0;
        while ((c = getchar()) != '\n')
        {
            message[length] = c;
            length++;
        }

        message[length] = '\0';

        if (length == 0) continue;

        if (length > 0 && message[0] == '/'){
            if(strcmp(message, "/disconnect") == 0)
            {
                puts("Disconnecting from the server");
                write(sockfd, "DISCONNECT", strlen("DISCONNECT"));
                *running = 0;
                break;
            }
            if(strcmp(message, "/close") == 0)
            {
                //puts("Disconnecting from the server");
                write(sockfd, "CLOSE", strlen("CLOSE"));
                continue;
            }  
        }
        
        // clearing the line
        puts("\033[2A");
        puts("\033[2K");

        write(sockfd, "MSG", strlen("MSG"));

        while(*fake_semaphore == 0);
                    
        write(sockfd, message, length);

        *fake_semaphore = 0;

        
    }
    close(sockfd);
    pthread_exit(NULL);
}



int main(int argc, char *argv[]) {
    int sockfd = 0, n = 0;
    int* running = (int*)malloc(sizeof(int));
    if (running == NULL) return 1;
    char recvBuff[2000];
    struct sockaddr_in serv_addr; 
    int* fake_semaphore = (int*)malloc(sizeof(int));
    if (fake_semaphore == NULL) return 1;

    memset(recvBuff, '0',sizeof(recvBuff));
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Error : Could not create socket \n");
        return 1;
    } 

    memset(&serv_addr, '0', sizeof(serv_addr)); 

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(5000); 
    serv_addr.sin_addr.s_addr = inet_addr(OUR_ADDRESS);

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
    conn_info* connection_info = (conn_info*)malloc(sizeof(conn_info));
    if (connection_info == NULL) return 1;
    *running = 1;
    *fake_semaphore = 0;
    connection_info->sockfd = sockfd;
    connection_info->running = running;
    connection_info->fake_semaphore = fake_semaphore;
    pthread_t input_thread, output_thread;
    pthread_create(&output_thread, NULL, outcoming_message_handling, (void*)connection_info);
    pthread_create(&input_thread, NULL, incoming_message_handling, (void*)connection_info);
    
    pthread_join(input_thread, NULL);
    pthread_join(output_thread, NULL);    

    return 0;
}
