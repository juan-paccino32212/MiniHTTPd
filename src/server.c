
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>

#include "server.h"

#define SERVER_PORT 8080
#define MAX_EVENTS 64
#define READ_BUFFER_SIZE 4096
#define MAX_READ_BUFFER 16384

/* Configurar un descriptor como no bloqueante */
static int set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);

    if (flags == -1)
        return -1;

    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

/* Agregar datos al buffer del cliente */
static int append_client_data(Client *client,
                              const char *data, size_t size)
{
    if (size > MAX_READ_BUFFER - client->read_size)
        return -1;

    size_t needed = client->read_size + size;

    if (needed > client->read_capacity) {
        size_t capacity = client->read_capacity ?
                          client->read_capacity : READ_BUFFER_SIZE;

        while (capacity < needed)
            capacity *= 2;

        if (capacity > MAX_READ_BUFFER)
            capacity = MAX_READ_BUFFER;

        char *buffer = realloc(client->read_buffer,
                               capacity + 1);

        if (!buffer)
            return -1;

        client->read_buffer = buffer;
        client->read_capacity = capacity;
    }

    memcpy(client->read_buffer + client->read_size,
           data, size);

    client->read_size += size;
    client->read_buffer[client->read_size] = '\0';

    return 0;
}

/* Recibir datos de un cliente */
static int handle_client_read(Client *client)
{
    char buffer[READ_BUFFER_SIZE];

    while (1) {
        ssize_t bytes = recv(client->fd, buffer,
                             sizeof(buffer), 0);

        if (bytes > 0) {
            if (append_client_data(client, buffer,
                                   (size_t)bytes) == -1) {
                fprintf(stderr, "Buffer del cliente excedido\n");
                return -1;
            }

            printf("Cliente fd=%d: %zd bytes recibidos "
                   "(total: %zu)\n",
                   client->fd, bytes, client->read_size);

            continue;
        }

        if (bytes == 0)
            return -1;

        if (errno == EINTR)
            continue;

        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return 0;

        perror("recv");
        return -1;
    }
}

/* Cerrar un cliente y aplazar la liberacion de su estructura */
static void close_client(int epoll_fd, Client *client,
                         Client **clients, Client **pending)
{
    if (client->closing)
        return;

    client->closing = 1;
    client->state = CLIENT_CLOSING;

    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client->fd, NULL);
    close(client->fd);
    client->fd = -1;

    Client **current = clients;

    while (*current && *current != client)
        current = &(*current)->next;

    if (*current == client)
        *current = client->next;

    free(client->read_buffer);
    client->read_buffer = NULL;

    client->next = *pending;
    *pending = client;

    printf("Cliente desconectado\n");
}

/* Liberar estructuras pendientes */
static void free_pending_clients(Client **pending)
{
    while (*pending) {
        Client *client = *pending;
        *pending = client->next;
        free(client);
    }
}

/* Aceptar conexiones entrantes */
static void accept_clients(int server_fd, int epoll_fd,
                           Client **clients)
{
    while (1) {
        int client_fd = accept(server_fd, NULL, NULL);

        if (client_fd == -1) {
            if (errno == EINTR)
                continue;

            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;

            perror("accept");
            break;
        }

        if (set_nonblocking(client_fd) == -1) {
            perror("fcntl client");
            close(client_fd);
            continue;
        }

        Client *client = calloc(1, sizeof(Client));

        if (!client) {
            perror("calloc");
            close(client_fd);
            continue;
        }

        client->fd = client_fd;
        client->state = CLIENT_READING;

        struct epoll_event event = {0};

        event.events = EPOLLIN | EPOLLRDHUP;
        event.data.ptr = client;

        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD,
                      client_fd, &event) == -1) {
            perror("epoll_ctl client");
            close(client_fd);
            free(client);
            continue;
        }

        client->next = *clients;
        *clients = client;

        printf("Cliente conectado: fd=%d\n", client_fd);
    }
}

/* Inicializar y ejecutar el servidor */
int server_start(void)
{
    int server_fd, epoll_fd;
    int reuse = 1;

    Client *clients = NULL;
    Client *pending = NULL;

    struct sockaddr_in address;
    struct epoll_event event;
    struct epoll_event events[MAX_EVENTS];

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd == -1) {
        perror("socket");
        return -1;
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,
                   &reuse, sizeof(reuse)) == -1) {
        perror("setsockopt");
        close(server_fd);
        return -1;
    }

    memset(&address, 0, sizeof(address));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(SERVER_PORT);

    if (bind(server_fd, (struct sockaddr *)&address,
             sizeof(address)) == -1) {
        perror("bind");
        close(server_fd);
        return -1;
    }

    if (listen(server_fd, SOMAXCONN) == -1) {
        perror("listen");
        close(server_fd);
        return -1;
    }

    if (set_nonblocking(server_fd) == -1) {
        perror("fcntl server");
        close(server_fd);
        return -1;
    }

    epoll_fd = epoll_create1(0);

    if (epoll_fd == -1) {
        perror("epoll_create1");
        close(server_fd);
        return -1;
    }

    memset(&event, 0, sizeof(event));

    event.events = EPOLLIN;
    event.data.ptr = NULL;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD,
                  server_fd, &event) == -1) {
        perror("epoll_ctl server");
        close(epoll_fd);
        close(server_fd);
        return -1;
    }

    printf("MiniHTTPd escuchando en el puerto %d...\n",
           SERVER_PORT);

    while (1) {
        int count = epoll_wait(epoll_fd, events,
                               MAX_EVENTS, -1);

        if (count == -1) {
            if (errno == EINTR)
                continue;

            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < count; i++) {

            if (events[i].data.ptr == NULL) {
                accept_clients(server_fd, epoll_fd, &clients);
                continue;
            }

            Client *client = events[i].data.ptr;

            if (client->closing)
                continue;

            if (events[i].events & (EPOLLIN | EPOLLRDHUP)) {
                if (handle_client_read(client) == -1) {
                    close_client(epoll_fd, client,
                                 &clients, &pending);
                    continue;
                }
            }

            if (!client->closing &&
                (events[i].events & (EPOLLERR | EPOLLHUP))) {
                close_client(epoll_fd, client,
                             &clients, &pending);
            }
        }

        free_pending_clients(&pending);
    }

    while (clients) {
        Client *client = clients;
        clients = client->next;

        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client->fd, NULL);
        close(client->fd);
        free(client->read_buffer);
        free(client);
    }

    free_pending_clients(&pending);

    close(epoll_fd);
    close(server_fd);

    return -1;
}