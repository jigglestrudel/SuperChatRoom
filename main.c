#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#define CLIENT_LIMIT 8


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
	int* client_connection_array = ((connection_info*)conn_info)->client_connection_array; 

	/* Get the socket descriptor */
	int sock = ((connection_info*)conn_info)->connfd;
	int read_size;
	char *message , client_message[2000];
	 
	/* Send some messages to the client */

	while(1) {
		// ask client for the name
		message = "ASK_NAME";
		write(sock , message , strlen(message));

		// await client nickname
		read_size = recv(sock , client_message , 2000 , 0);
		client_message[read_size] = '\0';

		// check if name available
		if (check_name_availibility(client_number, client_message, name_table, client_connection_array)){
			char* new_name = (char*)malloc(sizeof(char)*strlen(client_message));
			strcpy(new_name, client_message);
			break;
		}
	}


	 
	message = "Now type something and i shall repeat what you type\n";
	write(sock , message , strlen(message));
	
	message = "Empty line will close the connection\n";
	write(sock , message , strlen(message));  
	
	do {
		read_size = recv(sock , client_message , 2000 , 0);
		client_message[read_size] = '\0';
		
		/* Send the message back to client */
		write(sock , client_message , strlen(client_message));
		
		/* Clear the message buffer */
		memset(client_message, 0, 2000);
	} while(read_size > 2); /* Wait for empty line */
	
	fprintf(stderr, "Client disconnected\n"); 
	
	close(sock);
	pthread_exit(NULL);
}

typedef struct connection_info {
	int client_number;
	int connfd;
	char** name_table;
	int* client_connection_array; 
} connection_info;


int main(int argc, char *argv[]) {
	char* name_table[CLIENT_LIMIT];
	int client_connection_array[CLIENT_LIMIT];
	memset(client_connection_array, 0, CLIENT_LIMIT);


	int listenfd = 0, connfd = 0;
	struct sockaddr_in serv_addr; 

	pthread_t thread_id;

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	memset(&serv_addr, '0', sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(5000); 

	bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)); 

	listen(listenfd, 10); 
    

	for (;;) {
		connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);
		fprintf(stderr, "Connection accepted\n"); 
		// find free client_number
		int client_number = 0;
		for (client_number = 0; client_number <= CLIENT_LIMIT; client_number++)
		{
			if (client_number == CLIENT_LIMIT || client_connection_array[client_number] == 0)
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
			client_connection_array[client_number] = 1;
			connection_info* conn_info = (connection_info*)malloc(sizeof(connection_info));
			conn_info->client_number = client_number;
			conn_info->connfd = connfd;
			conn_info->client_connection_array = client_connection_array;
			conn_info->name_table = name_table;
			pthread_create(&thread_id, NULL, connection_handler , (void *) conn_info);
		}


		
	}
}

