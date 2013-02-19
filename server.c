/*
 * Our vulnerable server. It simply reads a name and a message from a client
 * before exiting.
 */
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <time.h>

#define PORTNUM 8001
#define BLENGTH 256
#define MLENGTH 128

/* Read the name of a client */
static void
read_name(int s, char *buffer, char *greeting)
{
  char *p;

  /* Construct a prompt */
  snprintf(buffer, BLENGTH, "Please enter your name:\n");

  /* Send the prompt to the client */
  send(s, (void *)buffer, BLENGTH, 0);

  /* Receive a response from the client */
  recv(s, (void *)buffer, BLENGTH, 0);

  /* Remove trailing '\n' from the response */
  p = buffer + strlen(buffer) - 1;
  *p = '\0';

  /* Transfer response to greeting */
  snprintf(greeting, BLENGTH, buffer);

  /* Construct greeting */
  strncat(greeting, ", you are most welcome!\n", BLENGTH);

  /* Send greeting to client */
  send(s, greeting, BLENGTH, 0);
}

/* Read a message from a client */
static void
read_message(int s, char *buffer, char *message)
{
  /* Construct a prompt */
  snprintf(buffer, BLENGTH, "Please enter your message:\n");

  /* Send the prompt to the client */
  send(s, (void *)buffer, BLENGTH, 0);

  /* Receive a response from the client */
  recv(s, (void *)buffer, BLENGTH, 0);

  /* Copy the response to a local buffer */
  strcpy(message, buffer);
}

/* Make an introduction */
static void
do_first_bit(int s)
{
  char greeting[BLENGTH];
  char buffer[BLENGTH];

  read_name(s, buffer, greeting);
}

/* Do the work */
static void
do_second_bit(int s)
{
  char message[MLENGTH];
  char buffer[BLENGTH];

  read_message(s, buffer, message);
}

/* One thread per connection */
void *
handler(void *n)
{
  int   s;

  /* Detach */
  pthread_detach(pthread_self());

  /* Cast parameter */
  s = *((int *)n);

  /* Make an introduction */
  do_first_bit(s);

  /* Do the work */
  do_second_bit(s);

  /* Close the socket */
  close(s);

  /* Free memory */
  free(n);

  return ((void *)NULL);
}

/* Connect and talk to a client */
int
main()
{
  struct sockaddr_in socketname, client;
  struct hostent *host;
  socklen_t clientlen = sizeof (client);
  pthread_t tid;
  int s, n, *c, optval = 1;

  /* We will always be localhost */
  if ((host = gethostbyname("localhost")) == NULL) {
    perror("gethostbyname()");
    exit(EXIT_FAILURE);
  }

  /* Fill in the socket address structure */
  memset((char *)&socketname, '\0', sizeof (socketname));
  socketname.sin_family = AF_INET;
  socketname.sin_port = htons(PORTNUM);
  memcpy((char *)&socketname.sin_addr, host->h_addr_list[0], host->h_length);

  /* Create an Internet family, stream socket */
  s = socket(AF_INET, SOCK_STREAM, 0);

  /* Did that work? */
  if (s < 0) {
    perror("socket()");
    exit(EXIT_FAILURE);
  }

  /* Allow binding if waiting on kernel to clean up */
  if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof (optval)) < 0) {
    perror("setsockopt()");
    exit(EXIT_FAILURE);
  }

  /* Now bind the address to our socket so it becomes visible */
  if (bind(s, (struct sockaddr *)&socketname, sizeof (socketname)) < 0) {
    perror("bind()");
    exit(EXIT_FAILURE);
  }

  /* Make our now visible socket available for connections */
  if (listen(s, 5)) {
    perror("listen()");
    exit(EXIT_FAILURE);
  }

  /* Loop forever */
  while (1) {

    /* Accept a connection */
    n = accept(s, (struct sockaddr *)&client, &clientlen);

    /* Did that work? */
    if (n < 0) {
      perror("accept()");
      exit(EXIT_FAILURE);
    }

    /* Allocate room for socket for this thread */
    c = malloc(sizeof (*c));

    /* Check we got some memory */
    if (!c) {
      perror("malloc()");
      exit(EXIT_FAILURE);
    }

    /* Initialise */
    *c = n;

    /* Have a conversation */
    pthread_create(&tid, NULL, handler, (void *)c);             
  }

  /* Close socket */
  close(s);

  return (0);
}

