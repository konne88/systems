/* Inspired by:
 * https://github.com/jasonish/libevent-examples/blob/master/echo-server/libevent_echosrv2.c
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

/* Port to listen on. */
#define SERVER_PORT 5555

#define FILE_NAME_SIZE 255

/**
 * A struct for client specific data, also includes pointer to create
 * a list of clients.
 *
 * In event based programming it is usually necessary to keep some
 * sort of object per client for state information.
 */
struct client {
	/* Events. We need 2 event structures, one for read event
	 * notification and the other for writing. */
	struct event ev_read;
	struct event ev_write;

    char  file_name [FILE_NAME_SIZE + 1];
    int   file_name_offset;
    char* file_data;
    int   file_data_size;
    int   file_data_offset;
};

/**
 * Set a socket to non-blocking mode.
 */
int
setnonblock(int fd)
{
	int flags;

	flags = fcntl(fd, F_GETFL);
	if (flags < 0)
		return flags;
	flags |= O_NONBLOCK;
	if (fcntl(fd, F_SETFL, flags) < 0)
		return -1;

        return 0;
}

/**
 * This function will be called by libevent when the client socket is
 * ready for reading.
 */
void
on_read(int fd, short ev, void *arg)
{
	struct client *client = (struct client *)arg;

    printf("Reading input!\n");
	
	int len = read(fd, &client->file_name + client->file_name_offset, 
                       FILE_NAME_SIZE - client->file_name_offset);
	if (len <= 0) {
		/* Client disconnected, remove the read event and the
		 * free the client structure. */
		printf("Socket failure, disconnecting client: %s",
        strerror(errno));
		close(fd);
		event_del(&client->ev_read);
		free(client);
		return;
	}
    client->file_name_offset += len;
    
    printf("Read %d\n", len);
  
    /* we received \n and know that the filename is complete */ 
    if (strchr(&client->file_name, '\n') != NULL) {
        printf("Done reading, read %s\n", &client->file_name);

     	/* Since we now have data that needs to be written back to the
     	 * client, add a write event. */
		event_del(&client->ev_read);
	    event_add(&client->ev_write, NULL);
        client->file_data = &client->file_name;
        client->file_data_size = FILE_NAME_SIZE;
    }
}

/**
 * This function will be called by libevent when the client socket is
 * ready for writing.
 */
void
on_write(int fd, short ev, void *arg)
{
	struct client *client = (struct client *)arg;

	/* Write the buffer.  A portion of the buffer may have been
	 * written in a previous write, so only write the remaining
	 * bytes. */
	int len = write(fd, client->file_data + client->file_data_offset,
                        client->file_data_size - client->file_data_offset);
	if (len == -1) {
		if (errno == EINTR || errno == EAGAIN) {
			/* The write was interrupted by a signal or we
			 * were not able to write any data to it,
			 * reschedule and return. */
			return;
		}
		else {
			/* Some other socket error occurred, exit. */
			event_del(&client->ev_write);
			err(1, "write");
		}
	}
	
    client->file_data_offset += len;
	if (client->file_data_offset == client->file_data_size) {
		/* All the data was written */
    	event_del(&client->ev_write);
        // TODO: free memory
    	// free(bufferq->buf);
    	free(client);		    
	}
}

/**
 * This function will be called by libevent when there is a connection
 * ready to be accepted.
 */
void
on_accept(int fd, short ev, void *arg)
{
	int client_fd;
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);
	struct client *client;

	/* Accept the new connection. */
	client_fd = accept(fd, (struct sockaddr *)&client_addr, &client_len);
	if (client_fd == -1) {
		warn("accept failed");
		return;
	}

	/* Set the client socket to non-blocking mode. */
	if (setnonblock(client_fd) < 0)
		warn("failed to set client socket non-blocking");

	/* We've accepted a new client, allocate a client object to
	 * maintain the state of this client. */
	client = calloc(1, sizeof(*client));
	if (client == NULL)
		err(1, "malloc failed");

	/* Setup the read event, libevent will call on_read() whenever
	 * the clients socket becomes read ready.  We also make the
	 * read event persistent so we don't have to re-add after each
	 * read. */
	event_set(&client->ev_read, client_fd, EV_READ|EV_PERSIST, on_read, client);

	/* Setting up the event does not activate, add the event so it
	 * becomes active. */
	event_add(&client->ev_read, NULL);

	/* Create the write event, but don't add it until we have
	 * something to write. */
	event_set(&client->ev_write, client_fd, EV_WRITE|EV_PERSIST, on_write, client);

	printf("Accepted connection from %s\n",
               inet_ntoa(client_addr.sin_addr));
}

int 
main(int argc, char **argv)
{
	int listen_fd;
	struct sockaddr_in listen_addr;
	int reuseaddr_on = 1;

	/* The socket accept event. */
	struct event ev_accept;

	/* Initialize libevent. */
	event_init();

	/* Create our listening socket. This is largely boiler plate
	 * code that I'll abstract away in the future. */
	listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_fd < 0)
		err(1, "listen failed");
	if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_on, 
		sizeof(reuseaddr_on)) == -1)
		err(1, "setsockopt failed");
	memset(&listen_addr, 0, sizeof(listen_addr));
	listen_addr.sin_family = AF_INET;
	listen_addr.sin_addr.s_addr = INADDR_ANY;
	listen_addr.sin_port = htons(SERVER_PORT);
	if (bind(listen_fd, (struct sockaddr *)&listen_addr,
		sizeof(listen_addr)) < 0)
		err(1, "bind failed");
	if (listen(listen_fd, 5) < 0)
		err(1, "listen failed");

	/* Set the socket to non-blocking, this is essential in event
	 * based programming with libevent. */
	if (setnonblock(listen_fd) < 0)
		err(1, "failed to set server socket to non-blocking");

	/* We now have a listening socket, we create a read event to
	 * be notified when a client connects. */
	event_set(&ev_accept, listen_fd, EV_READ|EV_PERSIST, on_accept, NULL);
	event_add(&ev_accept, NULL);

	/* Start the libevent event loop. */
	event_dispatch();

	return 0;
}
