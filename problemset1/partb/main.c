/*
; thread pool (pthreads)

; worker takes file name, read file, returns a pointer to the file

main 

; args : ip, port



; open tcp connection
; read file path, load file, return over tcp
; close tcp connection
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <errno.h>

#define TRUE             1
#define FALSE            0

void error(char* msg) {
  perror(msg);
  exit(-1);
}

int main (int argc, char *argv[])
{

  int port = 12345
  int listen_sd = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_sd < 0)
    error("socket() failed");

  /*************************************************************/
  /* Allow socket descriptor to be reuseable                   */
  /*************************************************************/
  /*
  rc = setsockopt(listen_sd, SOL_SOCKET,  SO_REUSEADDR,
                  (char *)&on, sizeof(on));
  if (rc < 0)
  {
    perror("setsockopt() failed");
    close(listen_sd);
    exit(-1);
  }
  */

  /*************************************************************/
  /* Set socket to be nonblocking. All of the sockets for    */
  /* the incoming connections will also be nonblocking since  */
  /* they will inherit that state from the listening socket.   */
  /*************************************************************/
  if (ioctl(listen_sd, FIONBIO, (char *)&on) < 0) {
    close(listen_sd);
    error("ioctl() failed");
  }

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family      = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port        = htons(port);
  if (bind(listen_sd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    close(listen_sd);
    error("bind() failed");
  }

  if (listen(listen_sd, 32) < 0) {
    close(listen_sd);
    error("listen() failed");
  }

  struct pollfd fds[200];
  memset(fds, 0 , sizeof(fds));

  int nfds = 1;
  fds[0].fd = listen_sd;
  fds[0].events = POLLIN;

  int    len, rc, on = 1;
  int    new_sd = -1;
  int    desc_ready, end_server = FALSE, compress_array = FALSE;
  int    close_conn;
  char   buffer[80];
  int  current_size = 0, i, j;



  /*************************************************************/
  /* Loop waiting for incoming connects or for incoming data   */
  /* on any of the connected sockets.                          */
  /*************************************************************/
  do {
    printf("Waiting on poll()...\n");
    if (poll(fds, nfds, -1) <= 0) {
      error("poll() failed");
      break;
    }

    current_size = nfds;
    for (i = 0; i < current_size; i++)
    {
      if(fds[i].revents == 0)
        continue;

      if (fds[i].fd == listen_sd)
      {
        printf("  Listening socket is readable\n");

        do {
          new_sd = accept(listen_sd, NULL, NULL);
          if (new_sd < 0)
          {
            if (errno != EWOULDBLOCK)
            {
              perror("  accept() failed");
              end_server = TRUE;
            }
            break;
          }

          /*****************************************************/
          /* Add the new incoming connection to the            */
          /* pollfd structure                                  */
          /*****************************************************/
          printf("  New incoming connection - %d\n", new_sd);
          fds[nfds].fd = new_sd;
          fds[nfds].events = POLLIN;
          nfds++;

          /*****************************************************/
          /* Loop back up and accept another incoming          */
          /* connection                                        */
          /*****************************************************/
        } while (new_sd != -1);
      }

      /*********************************************************/
      /* This is not the listening socket, therefore an        */
      /* existing connection must be readable                  */
      /*********************************************************/

      else
      {
        printf("  Descriptor %d is readable\n", fds[i].fd);
        close_conn = FALSE;
        /*******************************************************/
        /* Receive all incoming data on this socket            */
        /* before we loop back and call poll again.            */
        /*******************************************************/

        do
        {
          /*****************************************************/
          /* Receive data on this connection until the         */
          /* recv fails with EWOULDBLOCK. If any other        */
          /* failure occurs, we will close the                 */
          /* connection.                                       */
          /*****************************************************/
          rc = recv(fds[i].fd, buffer, sizeof(buffer), 0);
          if (rc < 0)
          {
            if (errno != EWOULDBLOCK)
            {
              perror("  recv() failed");
              close_conn = TRUE;
            }
            break;
          }

          /*****************************************************/
          /* Check to see if the connection has been           */
          /* closed by the client                              */
          /*****************************************************/
          if (rc == 0)
          {
            printf("  Connection closed\n");
            close_conn = TRUE;
            break;
          }

          /*****************************************************/
          /* Data was received                                 */
          /*****************************************************/
          len = rc;
          printf("  %d bytes received\n", len);

          /*****************************************************/
          /* Echo the data back to the client                  */
          /*****************************************************/
          rc = send(fds[i].fd, buffer, len, 0);
          if (rc < 0)
          {
            perror("  send() failed");
            close_conn = TRUE;
            break;
          }

        } while(TRUE);

        /*******************************************************/
        /* If the close_conn flag was turned on, we need       */
        /* to clean up this active connection. This           */
        /* clean up process includes removing the              */
        /* descriptor.                                         */
        /*******************************************************/
        if (close_conn)
        {
          close(fds[i].fd);
          fds[i].fd = -1;
          compress_array = TRUE;
        }


      }  /* End of existing connection is readable             */
    } /* End of loop through pollable descriptors              */

    /***********************************************************/
    /* If the compress_array flag was turned on, we need       */
    /* to squeeze together the array and decrement the number  */
    /* of file descriptors. We do not need to move back the    */
    /* events and revents fields because the events will always*/
    /* be POLLIN in this case, and revents is output.          */
    /***********************************************************/
    if (compress_array)
    {
      compress_array = FALSE;
      for (i = 0; i < nfds; i++)
      {
        if (fds[i].fd == -1)
        {
          for(j = i; j < nfds; j++)
          {
            fds[j].fd = fds[j+1].fd;
          }
          nfds--;
        }
      }
    }

  } while (end_server == FALSE); /* End of serving running.    */

  /*************************************************************/
  /* Clean up all of the sockets that are open                  */
  /*************************************************************/
  for (i = 0; i < nfds; i++)
  {
    if(fds[i].fd >= 0)
      close(fds[i].fd);
  }
}




