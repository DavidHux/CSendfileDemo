/* Client code */
/* TODO : Modify to meet your need */
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <time.h>
#include <pthread.h>

#define PORT_NUMBER     5000
#define SERVER_ADDRESS  "::1"
#define FILENAME        "receivedata"

int main(int argc, char **argv)
{
        int client_socket;
        ssize_t len;
        struct sockaddr_in remote_addr;
        char buffer[BUFSIZ];
	char buffer_filename[BUFSIZ];
        int file_size;
        FILE *received_file;
        int remain_data = 0;
	char* server_address = SERVER_ADDRESS;
	char if_receive_file[20];
	struct timespec start, end;
	if(argc > 1){
	    server_address = argv[1];
	}

        /* Zeroing remote_addr struct */
        memset(&remote_addr, 0, sizeof(remote_addr));

        /* Construct remote_addr struct */
        remote_addr.sin_family = AF_INET;
        inet_pton(AF_INET, server_address, &(remote_addr.sin_addr));
        remote_addr.sin_port = htons(PORT_NUMBER);

        /* Create client socket */
        client_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (client_socket == -1)
        {
                fprintf(stderr, "Error creating socket --> %s\n", strerror(errno));

                exit(EXIT_FAILURE);
        }

        /* Connect to the server */
        if (connect(client_socket, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr)) == -1)
        {
                fprintf(stderr, "Error on connect --> %s\n", strerror(errno));

                exit(EXIT_FAILURE);
        }

        /* Receiving file size */
        int lenR = recv(client_socket, buffer, BUFSIZ, 0);
        file_size = atoi(buffer);
	//printf("receive file size, %s %d\n", buffer, lenR);
	//recv(client_socket, buffer_filename, BUFSIZ, 0);

        fprintf(stdout, "\nFilename: %s, File size : %d. Receive? (yes/no/Y/N/y/n)\n", buffer+256, file_size);
	scanf("%s", if_receive_file);
	if(!(strcmp(if_receive_file, "yes") == 0 || strcmp(if_receive_file, "Y") == 0 || strcmp(if_receive_file, "y") == 0)) {
            printf("receiving file rejected.\n");
	    send(client_socket, "", 0, 0);
	    exit(EXIT_FAILURE);
	} else {
	    send(client_socket, "YES", 3, 0);
	}

        received_file = fopen(FILENAME, "w");
        if (received_file == NULL)
        {
                fprintf(stderr, "Failed to open file foo --> %s\n", strerror(errno));

                exit(EXIT_FAILURE);
        }

        remain_data = file_size;

        while (((len = recv(client_socket, buffer, BUFSIZ, 0)) > 0) && (remain_data > 0))
        {
                fwrite(buffer, sizeof(char), len, received_file);
                remain_data -= len;
		clock_gettime(CLOCK_MONOTONIC_RAW, &end);
                unsigned long delta_us = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;
		printf("%d ....\n", delta_us);
                printf("\rFile total %d bytes, received %d bytes, remain %d%, speed %d k/s... ", file_size, file_size - remain_data, (int)((float)remain_data / file_size * 100), len  / delta_us);
		clock_gettime(CLOCK_MONOTONIC_RAW, &start);
        }
        fclose(received_file);

        close(client_socket);

        return 0;
}
