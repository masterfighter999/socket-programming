#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>

#define MAXBUFFSZ 1024
#define LISTENQ 10
#define SA struct sockaddr

typedef struct
{
    int data[100];
    int count;
    double mean;
} ClientData;

typedef struct
{
    int sd;
    struct sockaddr_in cid;
    ClientData clientData;
} cli_ctxt_t;

int num_conn = 0;

void proc_serv_data(cli_ctxt_t *cli)
{
    ssize_t n;
    char recv_buff[MAXBUFFSZ], send_buff[MAXBUFFSZ];
    int sockfd;

    sockfd = cli->sd;

    while (1)
    {
        n = recv(sockfd, recv_buff, sizeof(recv_buff), 0);

        if (n > 0)
        {
            recv_buff[n] = '\0'; // Null-terminate received data

            // Check if the server wants to stop the stream
            if (strncmp(recv_buff, "STOP", 4) == 0)
            {
                printf("Client %s:%d requested to stop the stream.\n", inet_ntoa(cli->cid.sin_addr), ntohs(cli->cid.sin_port));
                break;
            }

            int num = atoi(recv_buff); // Convert received string to integer

            if (num >= 0 && num <= 1024)
            {
                // Update client's data
                cli->clientData.data[cli->clientData.count] = num;
                cli->clientData.count++;

                // Compute mean
                double sum = 0.0;
                for (int i = 0; i < cli->clientData.count; i++)
                {
                    sum += cli->clientData.data[i];
                }
                cli->clientData.mean = sum / cli->clientData.count;

                // Send mean value back to the client
                sprintf(send_buff, "Mean: %.2f\n", cli->clientData.mean);
                send(sockfd, send_buff, strlen(send_buff), 0);
            }
            else
            {
                strcpy(send_buff, "Invalid number. Please enter a number between 0 and 1024.\n");
                send(sockfd, send_buff, strlen(send_buff), 0);
            }
        }
        else if (n == 0)
        {
            // Client closed connection
            printf("Client %s:%d closed connection.\n", inet_ntoa(cli->cid.sin_addr), ntohs(cli->cid.sin_port));
            break;
        }
        else
        {
            perror("Recv error");
            break;
        }
    }

    close(sockfd);
    printf("Connection with client %s:%d closed.\n", inet_ntoa(cli->cid.sin_addr), ntohs(cli->cid.sin_port));
}

void child_proc_term_hdlr(int signo)
{
    pid_t pid;
    int stat;

    pid = waitpid(pid, &stat, WNOHANG);
    printf("Child %d terminated\n", pid);
    num_conn--;
    if (num_conn == 0)
    {
        printf("All sockets disconnected..\n");
        exit(0);
    }
}

int main(int argc, char **argv)
{
    int listenfd, connfd;
    pid_t childpid;
    socklen_t clilen;
    struct sockaddr_in cliaddr, servaddr;
    cli_ctxt_t cli_ctxt[10]; /* Maximum number of connected clients = 10 */
    void child_proc_term_hdlr(int);

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(atoi(argv[1]));

    if (bind(listenfd, (SA *)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("Bind error");
        exit(EXIT_FAILURE);
    }

    if (listen(listenfd, LISTENQ) < 0)
    {
        perror("Listen error");
        exit(EXIT_FAILURE);
    }

    signal(SIGCHLD, child_proc_term_hdlr);
    printf("Server started. Listening on port %s\n", argv[1]);

    for (;;)
    {
        clilen = sizeof(cliaddr);
        connfd = accept(listenfd, (SA *)&cliaddr, &clilen);

        if (connfd < 0)
        {
            perror("Accept error");
            continue;
        }

        memset(&cli_ctxt[num_conn], 0, sizeof(cli_ctxt_t));
        cli_ctxt[num_conn].sd = connfd;
        cli_ctxt[num_conn].cid.sin_addr = cliaddr.sin_addr;
        cli_ctxt[num_conn].cid.sin_port = cliaddr.sin_port;

        if ((childpid = fork()) == 0)
        {
            close(listenfd);
            proc_serv_data(&cli_ctxt[num_conn]);
            exit(EXIT_SUCCESS);
        }
        else if (childpid > 0)
        {
            printf("Client connected: %s:%d (PID: %d)\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port), childpid);
            num_conn++;
        }
        else
        {
            perror("Fork error");
            close(connfd);
        }
    }

    close(listenfd);
    return 0;
}

