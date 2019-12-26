#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#include <poll.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>

#define INVALID_PORT 1
#define BIND_ERROR 2
#define MEMORY_ERROR 3
#define POLL_ERROR 4
#define FILE_ERROR 5

struct pollfd* events = NULL;


void ctrl_c_handler(int sig)
{
    free(events);
    exit(0);
}


int make_new_client(struct pollfd** events, int nevents)
{
    struct pollfd* tmp = NULL;
    int fd;
    
    tmp = (struct pollfd*) realloc(*events, sizeof(struct pollfd*) * (nevents + 2));
    if(tmp == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for new client.\n");
        return MEMORY_ERROR;
    }

    fd = accept(tmp[0].fd, NULL, NULL);
    if(fd == -1)
    {
        perror("at connect");
        return FILE_ERROR;
    }

    tmp[nevents + 1].fd = fd;
    tmp[nevents + 1].events = POLLIN | POLLERR | POLLPRI;
    tmp[nevents + 1].revents = 0;

    *events = tmp;
    return 0;
}


int main(int argc, char** argv)
{
    int port;
    int tcp_connections_socket;

    struct sockaddr_in addr;

    int num_clients = 0;


    signal(SIGINT, ctrl_c_handler);

    if(argc < 2)
    {
        fprintf(stderr, "Should specify port.\n");
        return INVALID_PORT;
    }

    port = atoi(argv[1]);
    if(port < 1024 || port > 65000)
    {
        fprintf(stderr, "Invalid port\n.");
        return INVALID_PORT;
    }

    tcp_connections_socket = socket(PF_INET, SOCK_STREAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(tcp_connections_socket, (struct sockaddr*) &addr, sizeof(addr)) == -1)
    {
        fprintf(stderr, "Failed to bind port %d:%s\n", port, strerror(errno));
        return BIND_ERROR;
    }

    listen(tcp_connections_socket, 5);

    events = (struct pollfd*) malloc(sizeof(struct pollfd));
    if(events == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for events.\n");
        return MEMORY_ERROR;
    }

    events[0].fd = tcp_connections_socket;
    events[0].events = POLLIN || POLLPRI;
    events[0].revents = 0;

    while(1)
    {
        int ret_poll;
        int i;
        ret_poll = poll(events, num_clients + 1, 200);
        if(ret_poll < 0)
        {
            perror("at poll");
            return POLL_ERROR;
        }

        if(ret_poll == 0)
        {
            continue;
        }

        if(events[0].revents != 0)
        {
            int err;
            printf("client connected...\n");
            if((err = make_new_client(&events, num_clients)) != 0)
            {
                fprintf(stderr, "Failed to create new client entity.\n");
                return err;
            }
            num_clients++;
            events[0].revents = 0;
            ret_poll--;
        }

        for(i = 1; i < num_clients + 1; i++)
        {
            if(events[i].fd != -1 && events[i].revents != 0)
            {
                char byte;
                if(read(events[i].fd, &byte, 1) == 0)
                {
                    close(events[i].fd);
                    events[i].fd = -1;
                }
                printf("client %d: '%c'\n", i, byte);
            }
        }

        /*
         * TODO: exclude closed connections from events array.
         *       (swap closed one and last & realloc)
         */

        

    }

    return 111;
}