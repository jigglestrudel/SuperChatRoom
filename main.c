#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include "queue.c"

#define CLIENT_LIMIT 8

typedef struct connection_info {
	int client_number;
	int socket;
	char** name_table;
	int* is_client_connected; 
	sem_t* queue_semaphore;
	queue* messages_queue;
} connection_info;

typedef struct messages_info {
	int socket;
	int* clients_sockets;
	int* is_client_connected;
	char** name_table;
	queue* messages_queue;
} messages_info;

void* send_messages(void* clients) {
	int socket = ((messages_info*)clients)->socket;
	int* client_sockets = ((messages_info*)clients)->clients_sockets;
	int* is_client_connected = ((messages_info*)clients)->is_client_connected;
	char** name_table = ((messages_info*)clients)->name_table;
	queue* messages_to_send = ((messages_info*)clients)->messages_queue;
	char client_message[2000];
	for (;;) {
		if (messages_to_send->start != NULL) {
			node* message_to_send = messages_to_send->start;
			for (int i = 0; i < CLIENT_LIMIT; i++) {
				//message isn't send if current user is author of message or isn't connected
				if (is_client_connected[i]) {

					//sending username sender to user
					char* message = name_table[message_to_send->user_number];
					write(client_sockets[i], message, strlen(message));

					//sending message to user
					write(client_sockets[i] , message_to_send->message , message_to_send->message_length);
				}
			}
		}
	}
}

int check_name_availibility(int client_number, char* chosen_name, char** name_table, int* connection_table){
	for (int i = 0; i < CLIENT_LIMIT; i++)
	{
		if (connection_table[i] == 1 && i != client_number)
		{
			if (strcmp(chosen_name, name_table[i]) == 0)
			{
				return 0;
			}
		}
	}
	return 1;
}


void* connection_handler(void *conn_info)
{
	// get client number and other info
	int client_number = ((connection_info*)conn_info)->client_number;
	char** name_table = ((connection_info*)conn_info)->name_table;
	int* is_client_connected = ((connection_info*)conn_info)->is_client_connected; 
	sem_t* queue_semaphore = ((connection_info*)conn_info)->queue_semaphore;
	queue* message_queue = ((connection_info*)conn_info)->messages_queue;

	/* Get the socket descriptor */
	int sock = ((connection_info*)conn_info)->socket;
	int read_size;
	char *message , client_message[2000];
	 
	/* Send some messages to the client */
	while(1) {
		// ask client for the name
		printf("Pytam o nazwę użytkownika klienta numer %i", client_number);
		message = "ASK_NAME";
		write(sock , message , strlen(message));

		// await client nickname
		read_size = recv(sock , client_message , 2000 , 0);
		client_message[read_size] = '\0';

		// check if name available
		if (check_name_availibility(client_number, client_message, name_table, is_client_connected)){
			char* new_name = (char*)malloc(sizeof(char)*strlen(client_message));
			strcpy(new_name, client_message);
			name_table[client_number] = new_name;
			message = "NAME_GOOD";
			write(sock, message, strlen(message));  //send feedback on username
			printf("Nazwa uzytkownika klienta nr %i jest prawidłowa", client_number);
			break;
		}
		printf("Nazwa użytkownika nie jest prawidłowa");
	}

	printf("Nastąpiło połączenie klienta numer %i", client_number);
	//send connection info to users
	sem_wait(queue_semaphore);
	add_new_message(message_queue, "Użytkownik połączył się", client_number, strlen(client_message));
	sem_post(queue_semaphore);
	 
	//waiting for messages from user and saving to queue
	do {
		read_size = recv(sock , client_message , 2000 , 0);
		client_message[read_size] = '\0';

		if (strcmp(client_message, "MSG") != 0) {
			printf("Otrzymano nieprawidłową wiadomość");
			return 1;
		}

		message = "READY";
		write(sock, message, strlen(message));
		printf("Wysłano READY do użytkownika numer %i", client_number);

		read_size = recv(sock , client_message , 2000 , 0);
		client_message[read_size] = '\0';

		char* new_message = (char*)malloc(sizeof(char)*strlen(client_message));
		strcpy(new_message, client_message);

		printf("Otrzymano wiadomość %s od użytkownika %s", new_message, name_table[client_number]);

		sem_wait(queue_semaphore);
		add_new_message(message_queue, new_message, client_number, strlen(client_message));
		sem_post(queue_semaphore);

		/* Clear the message buffer */
		memset(client_message, 0, 2000);
	} while(read_size > 2); /* Wait for empty line */
	
	fprintf(stderr, "Client disconnected\n"); 
	
	printf("Klient numer %i rozłącza się", client_number);
	//send disconnection info to users
	sem_wait(queue_semaphore);
	add_new_message(message_queue, "Użytkownik rozłączył się", client_number, strlen(client_message));
	sem_post(queue_semaphore);

	close(sock);
	pthread_exit(NULL);
}


int main(int argc, char *argv[]) {
	int listenfd = 0, connfd = 0;
	struct sockaddr_in serv_addr; 
	pthread_t thread_id;
	queue* messages_queue = create_queue();
	sem_t queue_semaphore;
	int return_value = sem_init(&queue_semaphore, 0, 1);

	char* name_table[CLIENT_LIMIT];
	for (int i = 0; i < CLIENT_LIMIT; i++) {
		name_table[i] = NULL;
	}
	int is_client_connected[CLIENT_LIMIT];
	int client_sockets[CLIENT_LIMIT];
	messages_info* connected_messages_info = (messages_info*)malloc(sizeof(messages_info));  //PAMIĘTAJ O FREE
	if (connected_messages_info == NULL) {
		printf("Nie udało się zaalokować pamięci do przechowywania informacji o przyłączonych użytkownikach");
		return 1;
	}
	memset(is_client_connected, 0, CLIENT_LIMIT * sizeof(int));
	memset(client_sockets, 0, CLIENT_LIMIT * sizeof(int));
	connected_messages_info->clients_sockets = &client_sockets;
	connected_messages_info->is_client_connected = &is_client_connected; 
	connected_messages_info->messages_queue = messages_queue;
	connected_messages_info->name_table = name_table;

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	memset(&serv_addr, '0', sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(5000); 

	bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)); 

	listen(listenfd, 10); 
    
	pthread_t sending_messages_thread_id;
	pthread_create(&sending_messages_thread_id, NULL, send_messages, (void*)connected_messages_info);	

	for (;;) {
		connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);
		fprintf(stderr, "Connection accepted\n"); 
		// find free client_number
		int client_number = 0;
		for (client_number = 0; client_number <= CLIENT_LIMIT; client_number++)
		{
			if (client_number == CLIENT_LIMIT || is_client_connected[client_number] == 0)
			{
				break;
			}
		}

		if (client_number == CLIENT_LIMIT)
		{
			printf("Client number limit reached. Disconnecting the client\n");
			close(connfd);
			continue;
		}
		else
		{
			is_client_connected[client_number] = 1;
			client_sockets[client_number] = connfd;
			connection_info* conn_info = (connection_info*)malloc(sizeof(connection_info));
			conn_info->client_number = client_number;
			conn_info->socket = connfd;
			conn_info->is_client_connected = is_client_connected;
			conn_info->name_table = name_table;
			conn_info->messages_queue = messages_queue;
			conn_info->queue_semaphore = &queue_semaphore;
			pthread_create(&thread_id, NULL, connection_handler, (void *) conn_info);
		}
	}
	//NA RAZIE NIE MA ŻADNEGO WYŁĄCZANIA SERWERA - CZY POWINNO BYĆ?
}

