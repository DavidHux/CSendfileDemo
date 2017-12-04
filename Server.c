/* Server code */
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
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <time.h>

#define PORT_NUMBER 5000
#define PORT_NUMBER_2 5001
#define SERVER_ADDRESS "fe80::e54:a5ff:fe52:7cfc"
#define FILE_TO_SEND "/home/ics/hux/fulldata.txt"
void *client_control(void *x_void_ptr);

int main(int argc, char **argv)
{
    int server_socket;
    int peer_socket;
    socklen_t sock_len;
    ssize_t len;
    struct sockaddr_in6 server_addr;
    struct sockaddr_in6 peer_addr;
    int fd;
    int sent_bytes = 0;
    char file_size[256];
    struct stat file_stat;
    off_t offset;
    int remain_data;
    char *filename = FILE_TO_SEND;
    struct timespec start, end;
    char ip6buf[INET6_ADDRSTRLEN];
    int speed = 500;
    int client_state[2] = {0};

    /* Create server socket */
    server_socket = socket(AF_INET6, SOCK_STREAM, 0);
    if (server_socket == -1)
    {
        fprintf(stderr, "Error creating socket --> %s", strerror(errno));

        exit(EXIT_FAILURE);
    }

    if (argc > 1)
    {
        if (argc > 2)
        {
            if (argc != 4)
            {
                printf("Usage: filename [-s speed]\n");
                exit(1);
            }
            int ia;
            for (ia = 1; ia < argc; ia++)
            {
                if (strcmp(argv[ia], "-s") == 0)
                {
                    if (!(argv[ia + 1][0] >= '0' && argv[ia + 1][0] <= '9'))
                    {
                        printf("Usage: filename [-s speed]\n");
                        exit(1);
                    }
                    speed = atoi(argv[ia + 1]);
                    ia++;
                }
                else
                {
                    filename = argv[ia];
                }
            }
        }
        else
        {
            filename = argv[1];
        }
    }

    /* Zeroing server_addr struct */
    memset(&server_addr, 0, sizeof(server_addr));
    /* Construct server_addr struct */
    server_addr.sin6_family = AF_INET6;
    //inet_pton(AF_INET6, SERVER_ADDRESS, &(server_addr.sin6_addr));
    server_addr.sin6_addr = in6addr_any;
    server_addr.sin6_port = htons(PORT_NUMBER);

    /* Bind */
    if ((bind(server_socket, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in6))) == -1)
    {
        fprintf(stderr, "Error on bind --> %s", strerror(errno));

        exit(EXIT_FAILURE);
    }

    /* Listening to incoming connections */
    if ((listen(server_socket, 5)) == -1)
    {
        fprintf(stderr, "Error on listen --> %s", strerror(errno));

        exit(EXIT_FAILURE);
    }

    fd = open(filename, O_RDONLY);
    if (fd == -1)
    {
        fprintf(stderr, "Error opening file --> %s", strerror(errno));

        exit(EXIT_FAILURE);
    }

    /* Get file stats */
    if (fstat(fd, &file_stat) < 0)
    {
        fprintf(stderr, "Error fstat --> %s", strerror(errno));

        exit(EXIT_FAILURE);
    }

    fprintf(stdout, "File Size: \n%d bytes\n", (int)file_stat.st_size);

    sock_len = sizeof(struct sockaddr_in6);
    /* Accepting incoming peers */
    peer_socket = accept(server_socket, (struct sockaddr *)&peer_addr, &sock_len);
    if (peer_socket == -1)
    {
        fprintf(stderr, "Error on accept --> %s", strerror(errno));

        exit(EXIT_FAILURE);
    }
    fprintf(stdout, "Accept peer --> %s\n", inet_ntop(AF_INET6, &(peer_addr.sin6_addr), ip6buf, INET6_ADDRSTRLEN));

    sprintf(file_size, "%d", (int)file_stat.st_size);

    /* Sending file size */
    len = send(peer_socket, file_size, sizeof(file_size), 0);
    //printf("send file size, %d\n", len);
    len = send(peer_socket, filename, strlen(filename), 0);
    //printf("send file name, %d\n", len);

    if (len < 0)
    {
        fprintf(stderr, "Error on sending greetings --> %s", strerror(errno));

        exit(EXIT_FAILURE);
    }

    char temp[10];
    int lene = recv(peer_socket, temp, 10, 0);
    if (lene == 0)
    {
        printf("client disconnected.\n");
        exit(0);
    }

    fprintf(stdout, "Server sent filename and file size\n", (int)len);

    pthread_t client_control_thread;

    /* create a second thread which executes inc_x(&x) */
    if (pthread_create(&client_control_thread, NULL, client_control, (void *)client_state))
    {
        fprintf(stderr, "Error creating thread\n");
        return 1;
    }
    offset = 0;
    remain_data = file_stat.st_size;
    /* Sending file data */
    int round = 0;

    clock_gettime(CLOCK_MONOTONIC_RAW, &start);
    while (((sent_bytes = sendfile(peer_socket, fd, &offset, BUFSIZ)) > 0) && (remain_data > 0))
    {
        remain_data -= sent_bytes;
        if ((offset + sent_bytes) > round * 1000 * speed)
        {
            clock_gettime(CLOCK_MONOTONIC_RAW, &end);
            unsigned long delta_us = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000;
            if (delta_us < 1000)
            {
                usleep(1000 * (1000 - delta_us));
                printf("\nServer sending file. Total size %d bytes, sent %d bytes, remain %d %, speed %d k/s, time %ds ...", file_stat.st_size, (int)offset + sent_bytes, (int)((float)remain_data * 100 / file_stat.st_size), speed, remain_data / speed / 1000);
            }
            else
            {
                printf("\nServer sending file. Total size %d bytes, sent %d bytes, remain %d %, speed %d k/s, time %ds ...", file_stat.st_size, (int)offset + sent_bytes, (int)((float)remain_data * 100 / file_stat.st_size), speed * 1000 / delta_us, remain_data / 1000 / (speed * 1000 / delta_us));
            }
            clock_gettime(CLOCK_MONOTONIC_RAW, &start);
            //fprintf(stdout, "2. Server sent %d bytes from file's data, offset is now : %d and remaining data = %d\n", sent_bytes, (int)offset, remain_data);
            round++;
        }
        if (client_state[1] == 1)
        {
            printf("\nclient paused\n");
            while (client_state[1] == 1)
                ;
            printf("continue...\n");
        }
    }

    close(peer_socket);
    close(server_socket);
    printf("\nSendfile finished.\n");

    return 0;
}
void *client_control(void *x_void_ptr)
{
    int *x_ptr = (int *)x_void_ptr;
    int sockfd, new_fd;
    socklen_t len;

    struct sockaddr_in6 my_addr, their_addr; // IPv6

    char buf[BUFSIZ];

    if ((sockfd = socket(PF_INET6, SOCK_STREAM, 0)) == -1)
    { // IPv6
        perror("socket2");
        exit(1);
    }

    bzero(&my_addr, sizeof(my_addr));
    my_addr.sin6_family = PF_INET6;           // IPv6
    my_addr.sin6_port = htons(PORT_NUMBER_2); // IPv6
    my_addr.sin6_addr = in6addr_any;          // IPv6
    //inet_pton(AF_INET6, SERVER_ADDRESS, &(my_addr.sin6_addr));

    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr_in6)) == -1)
    {
        perror("bind2");
        exit(1);
    }
    if (listen(sockfd, 5) == -1)
    {
        perror("listen2");
        exit(1);
    }
    if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &len)) == -1)
    {
        perror("accept2");
        exit(errno);
    }
    x_ptr[0] = 1;
    while (1)
    {
        if ((len = recv(new_fd, buf, BUFSIZ, 0)) > 0)
        {
            char a = buf[0];
            if (a == 'p')
            {
                x_ptr[1] = 1;
            }
            else if (a == 'q')
            {
                x_ptr[1] = 2;
            }
            else if (a == 'c')
            {
                x_ptr[1] = 0;
            }
            else
            {
                printf("receive client error: %s\n", buf);
            }
        }
    }
    printf("connection ended\n");
    /* the function must return something - NULL will do */
    return NULL;
}
