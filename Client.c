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
#include <net/if.h>

#define PORT_NUMBER 5000
#define PORT_NUMBER2 5001
#define SERVER_ADDRESS "::1"
#define FILENAME "receiveFile"
#define INTERFACE ""

void *sendfile_control(void *x_void_ptr);

char *server_address = SERVER_ADDRESS;
char *interface = INTERFACE;
int main(int argc, char **argv)
{
    int client_socket;
    ssize_t len;
    struct sockaddr_in6 remote_addr;
    char buffer[BUFSIZ];
    char buffer_filename[BUFSIZ];
    int file_size;
    FILE *received_file;
    int remain_data = 0;
    char if_receive_file[20];
    struct timespec start, end;
    char *filename1;

    int state = 0;
    if (argc > 2)
    {
        interface = argv[2];
    }

    if (argc > 1)
    {
        server_address = argv[1];
    }
    printf("server address; %s\n", server_address);

    /* Zeroing remote_addr struct */
    memset(&remote_addr, 0, sizeof(remote_addr));

    /* Construct remote_addr struct */
    remote_addr.sin6_family = AF_INET6;
    inet_pton(AF_INET6, server_address, &(remote_addr.sin6_addr));
    remote_addr.sin6_port = htons(PORT_NUMBER);
    if (strlen(interface) > 0)
    {
        remote_addr.sin6_scope_id = if_nametoindex(interface);
    }
    /* Create client socket */
    client_socket = socket(AF_INET6, SOCK_STREAM, 0);
    if (client_socket == -1)
    {
        fprintf(stderr, "Error creating socket --> %s\n", strerror(errno));

        exit(EXIT_FAILURE);
    }

    /* Connect to the server */
    if (connect(client_socket, (struct sockaddr *)&remote_addr, sizeof(remote_addr)) == -1)
    {
        fprintf(stderr, "Error on connect --> %s\n", strerror(errno));

        exit(EXIT_FAILURE);
    }

    /* Receiving file size */
    int lenR = recv(client_socket, buffer, BUFSIZ, 0);
    file_size = atoi(buffer);
    //printf("receive file size, %s %d\n", buffer, lenR);
    if (lenR <= 256)
    {
        recv(client_socket, buffer_filename, BUFSIZ, 0);
        filename1 = buffer_filename;
    }
    else
    {
        filename1 = buffer + 256;
    }
    fprintf(stdout, "\nFilename: %s, File size : %d. Receive? (yes/no/Y/N/y/n)\n", filename1, file_size);
    scanf("%s", if_receive_file);
    if (!(strcmp(if_receive_file, "yes") == 0 || strcmp(if_receive_file, "Y") == 0 || strcmp(if_receive_file, "y") == 0))
    {
        printf("receiving file rejected.\n");
        send(client_socket, "", 0, 0);
        exit(EXIT_FAILURE);
    }
    else
    {
        send(client_socket, "YES", 3, 0);
    }

    received_file = fopen(FILENAME, "w");
    if (received_file == NULL)
    {
        fprintf(stderr, "Failed to open file foo --> %s\n", strerror(errno));

        exit(EXIT_FAILURE);
    }

    pthread_t sendfile_control_thread;

    /* create a second thread which executes inc_x(&x) */
    if (pthread_create(&sendfile_control_thread, NULL, sendfile_control, &state))
    {
        fprintf(stderr, "Error creating thread\n");
        return 1;
    }

    remain_data = file_size;

    while ((len = recv(client_socket, buffer, BUFSIZ, 0)) > 0 && remain_data > 0)
    {
        fwrite(buffer, sizeof(char), len, received_file);
        remain_data -= len;
        clock_gettime(CLOCK_MONOTONIC_RAW, &end);
        unsigned long delta_us = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;
        //printf("%d ....\n", delta_us);
        printf("\rFile total %d bytes, received %d bytes, remain %d%, speed %d k/s, time %ds ... ", file_size, file_size - remain_data, (int)((float)remain_data / file_size * 100), len / delta_us, remain_data / 1000 / (len / delta_us + 1));
        clock_gettime(CLOCK_MONOTONIC_RAW, &start);
    }
    fclose(received_file);

    close(client_socket);
    printf("\nFile received -> reveiveFile\n");

    return 0;
}
/* this function is run by the second thread */
void *sendfile_control(void *x_void_ptr)
{
    int *x_ptr = (int *)x_void_ptr;

    /* increment x to 100 */
    int sockfd, len;
    struct sockaddr_in6 dest; // IPv6
    char buffer[BUFSIZ];

    if ((sockfd = socket(AF_INET6, SOCK_STREAM, 0)) < 0)
    { // IPv6
        perror("Socket");
        exit(errno);
    }

    bzero(&dest, sizeof(dest));
    dest.sin6_family = AF_INET6;          // IPv6
    dest.sin6_port = htons(PORT_NUMBER2); // IPv6
    if (strlen(interface) > 0)
    {
        dest.sin6_scope_id = if_nametoindex(interface);
    }
    if (inet_pton(AF_INET6, server_address, &dest.sin6_addr) < 0)
    { // IPv6
        perror(server_address);
        exit(errno);
    }

    if (connect(sockfd, (struct sockaddr *)&dest, sizeof(dest)) != 0)
    {
        perror("Connect ");
        exit(errno);
    }
    while (1)
    {
        char c = '0';
        scanf("%c", &c);
        if (c == 'p')
        {
            *x_ptr = 1;
            len = send(sockfd, &c, 1, 0);
            printf("\nPaused...");
        }
        else if (c == 'c')
        {
            *x_ptr = 0;
            len = send(sockfd, &c, 1, 0);
            printf("\nContinue...\n");
        }
        else if (c == 'q')
        {
            *x_ptr = 2;
            len = send(sockfd, &c, 1, 0);
            printf("\nQuit\n");
        }
    }
    /* the function must return something - NULL will do */
    return NULL;
}
