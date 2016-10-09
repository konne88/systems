/* Inspired by:
 * https://github.com/jasonish/libevent-examples/blob/master/echo-server/libevent_echosrv2.c
 * and
 * https://sourceforge.net/p/libevent-thread/code/ci/master/tree/
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <err.h>
#include <event.h>

#define MAX_FILE_NAME_SIZE 255
#define MAX_FILE_DATA_SIZE (2 * 1024 * 1024)

struct client {
    struct event ev_read;
    struct event ev_write;

    char  file_name [MAX_FILE_NAME_SIZE + 1];
    int   file_name_offset;

    char  file_data [MAX_FILE_DATA_SIZE];
    int   file_data_offset;
    int   file_data_size;
};

int setnonblock(int fd) {
    int flags = fcntl(fd, F_GETFL);
    if (flags < 0) 
        return flags;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void on_read(int fd, short ev, void *arg) {
    struct client *client = (struct client *)arg;
    int len = read(fd, &client->file_name + client->file_name_offset, 
                       MAX_FILE_NAME_SIZE - client->file_name_offset);
    if (len <= 0) {
        printf("Client error: %s\n", strerror(errno));
        close(fd);
        event_del(&client->ev_read);
        free(client);
        return;
    }
    client->file_name_offset += len;
    printf("Done receiving %d bytes of the filename.\n", len);

    char* loc = strchr(&client->file_name, '\n');
    if (loc != NULL) {
        /* we received \n and know that the filename is complete */ 
        *loc = '\0';
        printf("The filename is: '%s'\n", &client->file_name);
        event_del(&client->ev_read);
        event_add(&client->ev_write, NULL);

        FILE* f = fopen(&client->file_name, "rb");

        if (f == NULL) {
            printf("Error opening file: %s\n", strerror(errno));
        } else {
            client->file_data_size = fread(&client->file_data, 1, MAX_FILE_DATA_SIZE, f);
            printf("Done reading entire file with %d bytes from disk.\n", client->file_data_size);
            fclose(f);
        }
    }
}

void on_write(int fd, short ev, void *arg)
{
    struct client *client = (struct client *)arg;

    int len = write(fd, &client->file_data + client->file_data_offset,
                        client->file_data_size - client->file_data_offset);
    if (len == -1) {
        if (errno == EINTR || errno == EAGAIN) {
            /* The write was interrupted by a signal or we
             * were not able to write any data to it,
             * reschedule and return. */
            len = 0;
        }
        else {
            /* Some other socket error occurred, exit. */
            event_del(&client->ev_write);
            err(1, "writei");
        }
    }
    printf("Done sending %d bytes of the file.\n", len);
    
    client->file_data_offset += len;
    if (client->file_data_offset == client->file_data_size) {
        /* All the file data was written */
        event_del(&client->ev_write);
        close(fd);
        free(client);
        printf("Done sending entire file.\n");
    }
}

void on_accept(int fd, short ev, void *arg)
{
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd == -1) {
        warn("accept failed");
        return;
    }

    if (setnonblock(client_fd) < 0)
        warn("failed to set client socket non-blocking");

    struct client* client = calloc(1, sizeof(*client));
    if (client == NULL)
        err(1, "malloc failed");

    event_set(&client->ev_read, client_fd, EV_READ|EV_PERSIST, on_read, client);
    event_set(&client->ev_write, client_fd, EV_WRITE|EV_PERSIST, on_write, client);
    event_add(&client->ev_read, NULL);

    printf("Accepted connection from %s\n", inet_ntoa(client_addr.sin_addr));
}

int main(int argc, char **argv)
{
    int port = 5555;

    event_init();

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) 
        err(1, "listen failed");
    
    int reuseaddr_on = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_on, 
        sizeof(reuseaddr_on)) == -1)
        err(1, "setsockopt failed");
    
    struct sockaddr_in listen_addr;
    memset(&listen_addr, 0, sizeof(listen_addr));
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_addr.s_addr = INADDR_ANY;
    listen_addr.sin_port = htons(port);
    if (bind(listen_fd, (struct sockaddr *)&listen_addr,
        sizeof(listen_addr)) < 0)
        err(1, "bind failed");
    if (listen(listen_fd, 5) < 0)
        err(1, "listen failed");

    if (setnonblock(listen_fd) < 0)
        err(1, "failed to set server socket to non-blocking");

    struct event ev_accept;
    event_set(&ev_accept, listen_fd, EV_READ|EV_PERSIST, on_accept, NULL);
    event_add(&ev_accept, NULL);

    event_dispatch();

    return 0;
}
