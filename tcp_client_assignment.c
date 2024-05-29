#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAXBUFFSZ 1024

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <server_ip> <server_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int sockfd;
    struct sockaddr_in servaddr;
    char send_buff[MAXBUFFSZ], recv_buff[MAXBUFFSZ];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(argv[1]);
    servaddr.sin_port = htons(atoi(argv[2]));

    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("Connection to server failed");
        exit(EXIT_FAILURE);
    }

    printf("Connected to server at %s:%s\n", argv[1], argv[2]);
    printf("Enter integers between 0 and 1024. Type '-1' to end.\n");

    while (1)
    {
        int num;
        printf("Enter number: ");
        scanf("%d", &num);

        if (num >= 0 && num <= 1024)
        {
            sprintf(send_buff, "%d", num);
            send(sockfd, send_buff, strlen(send_buff), 0);

            ssize_t n = recv(sockfd, recv_buff, sizeof(recv_buff), 0);
            if (n > 0)
            {
                recv_buff[n] = '\0';
                printf("Server response: %s", recv_buff);
            }
            else if (n == 0)
            {
                printf("Server closed connection.\n");
                break;
            }
            else
            {
                perror("Recv error");
                break;
            }
        }
        else if (num < 0)
        {
            if (num == -1)
            {
                strcpy(send_buff, "STOP");
                send(sockfd, send_buff, strlen(send_buff), 0);
                printf("Stopping...\n");
                break;
            }
            printf("Number must be greater than or equal to 0.\n");
        }
        else if (num > 1024)
        {
            printf("Number must be less than or equal to 1024.\n");
        }
    }

    close(sockfd);
    printf("Connection closed.\n");
    return 0;
}

