#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "msg.h"

#define BUF 256

void Usage(char *progname);

int LookupName(char *name,
                unsigned short port,
                struct sockaddr_storage *ret_addr,
                size_t *ret_addrlen);

int Connect(const struct sockaddr_storage *addr,
             const size_t addrlen,
             int *ret_fd);

int
main(int argc, char **argv) {
  if (argc != 3) {
    Usage(argv[0]);
  }

  unsigned short port = 0;
  if (sscanf(argv[2], "%hu", &port) != 1) {
    Usage(argv[0]);
  }

  // Get an appropriate sockaddr structure.
  struct sockaddr_storage addr;
  size_t addrlen;
  if (!LookupName(argv[1], port, &addr, &addrlen)) {
    Usage(argv[0]);
  }

  // Connect to the remote host.
  int socket_fd;
  if (!Connect(&addr, addrlen, &socket_fd)) {
    Usage(argv[0]);
  }
 
 
  //vars for user input and server input
  int8_t choice, flag;
  struct msg m;
  flag = 1;
  struct msg temp;
  //loop while user doesnt want to exit
  while (flag){
  //get user choice
printf("Enter your choice (1 to put, 2 to get, 3 to delete, 0 to quit): ");
scanf("%" SCNd8 "%*c", &choice);
// check choice
switch (choice){
//put
case 1:
      // set choice for server
      m.type = choice;
      //get user input for struct 
      printf("Enter the student name: ");
      fgets(m.rd.name, MAX_NAME_LENGTH, stdin);
      
      m.rd.name[strlen(m.rd.name)-1] = '\0';
     
      printf("Enter the id: ");
     	scanf("%"SCNd32 "%*c", &(m.rd.id));
      // send data to server
      int wres = write(socket_fd, &m, sizeof(m));
      //check errors
      if (wres == 0) {
     printf("socket closed prematurely \n");
      close(socket_fd);
      return EXIT_FAILURE;
    }
    if (wres == -1) {
      if (errno == EINTR)
        continue;
      printf("socket write failure \n");
      close(socket_fd);
      return EXIT_FAILURE;
    }
      int res;
      // get data from server
      char readbuf[BUF];
      //check errors
      res = read(socket_fd, readbuf, BUF-1);
      if (res == 0) {
      printf("socket closed prematurely \n");
      close(socket_fd);
      return EXIT_FAILURE;
    }
    if (res == -1) {
      if (errno == EINTR)

      printf("socket read failure \n");
      close(socket_fd);
      return EXIT_FAILURE;
    }
      readbuf[res] = '\0';
      printf("%s\n", readbuf);
break;
// get
case 2:
      // set choice for server
      m.type = choice;
      //get user input for struct (only id)
      printf("Enter the id: ");
     	scanf("%"SCNd32 "%*c", &(m.rd.id));
      // send id to server
      wres = write(socket_fd, &m, sizeof(m));
      //check errors
      if (wres == 0) {
     printf("socket closed prematurely \n");
      close(socket_fd);
      return EXIT_FAILURE;
    }
    if (wres == -1) {
      if (errno == EINTR)
        continue;
      printf("socket write failure \n");
      close(socket_fd);
      return EXIT_FAILURE;
    }
      // get data from server
      res = read(socket_fd, &temp, sizeof(temp));
      if (res == 0) {
      printf("socket closed prematurely \n");
      close(socket_fd);
      return EXIT_FAILURE;
    }
    if (res == -1) {
      if (errno == EINTR)

      printf("socket read failure \n");
      close(socket_fd);
      return EXIT_FAILURE;
    }
      // if data exists then print data
      if(temp.rd.id == m.rd.id)
      {
        printf("name: %s\n", temp.rd.name);
        printf("id: %u\n", temp.rd.id);
      }
      // if data doesnt exist print failed
      else
      {
        printf("get failed\n");
      }
      break;
//delete
case 3:
// set choice for server
      m.type = choice;
      //get user input for struct (only id)
      printf("Enter the id: ");
     	scanf("%"SCNd32 "%*c", &(m.rd.id));
      // send id to server
      wres = write(socket_fd, &m, sizeof(m));
      //check errors
      if (wres == 0) {
     printf("socket closed prematurely \n");
      close(socket_fd);
      return EXIT_FAILURE;
    }
    if (wres == -1) {
      if (errno == EINTR)
        continue;
      printf("socket write failure \n");
      close(socket_fd);
      return EXIT_FAILURE;
    }
      // get data from server
      res = read(socket_fd, &temp, sizeof(temp));
      if (res == 0) {
      printf("socket closed prematurely \n");
      close(socket_fd);
      return EXIT_FAILURE;
    }
    if (res == -1) {
      if (errno == EINTR)

      printf("socket read failure \n");
      close(socket_fd);
      return EXIT_FAILURE;
    }
      // if data exists then print data
      if(temp.rd.id == m.rd.id)
      {
        printf("name: %s\n", temp.rd.name);
        printf("id: %u\n", temp.rd.id);
      }
      // if data doesnt exist print failed
      else
      {
        printf("delete failed\n");
      }
      break;
//exit
default:
flag = 0;
}
  }
 
 

  // Read something from the remote host.
  // Will only read BUF-1 characters at most.
  /*char readbuf[BUF];
  int res;

    res = read(socket_fd, readbuf, BUF-1);
    if (res == 0) {
      printf("socket closed prematurely \n");
      close(socket_fd);
      return EXIT_FAILURE;
    }
    if (res == -1) {
      if (errno == EINTR)

      printf("socket read failure \n");
      close(socket_fd);
      return EXIT_FAILURE;
    }
    readbuf[res] = '\0';
    printf("%s", readbuf);
 

  // Write something to the remote host.
  while (1) {
    int wres = write(socket_fd, readbuf, res);
    if (wres == 0) {
     printf("socket closed prematurely \n");
      close(socket_fd);
      return EXIT_FAILURE;
    }
    if (wres == -1) {
      if (errno == EINTR)
        continue;
      printf("socket write failure \n");
      close(socket_fd);
      return EXIT_FAILURE;
    }
    break;
  }*/

  // Clean up.
  close(socket_fd);
  return EXIT_SUCCESS;
}

void
Usage(char *progname) {
  printf("usage: %s  hostname port \n", progname);
  exit(EXIT_FAILURE);
}

int
LookupName(char *name,
                unsigned short port,
                struct sockaddr_storage *ret_addr,
                size_t *ret_addrlen) {
  struct addrinfo hints, *results;
  int retval;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  // Do the lookup by invoking getaddrinfo().
  if ((retval = getaddrinfo(name, NULL, &hints, &results)) != 0) {
    printf( "getaddrinfo failed: %s", gai_strerror(retval));
    return 0;
  }

  // Set the port in the first result.
  if (results->ai_family == AF_INET) {
    struct sockaddr_in *v4addr =
            (struct sockaddr_in *) (results->ai_addr);
    v4addr->sin_port = htons(port);
  } else if (results->ai_family == AF_INET6) {
    struct sockaddr_in6 *v6addr =
            (struct sockaddr_in6 *)(results->ai_addr);
    v6addr->sin6_port = htons(port);
  } else {
    printf("getaddrinfo failed to provide an IPv4 or IPv6 address \n");
    freeaddrinfo(results);
    return 0;
  }

  // Return the first result.
  assert(results != NULL);
  memcpy(ret_addr, results->ai_addr, results->ai_addrlen);
  *ret_addrlen = results->ai_addrlen;

  // Clean up.
  freeaddrinfo(results);
  return 1;
}

int
Connect(const struct sockaddr_storage *addr,
             const size_t addrlen,
             int *ret_fd) {
  // Create the socket.
  int socket_fd = socket(addr->ss_family, SOCK_STREAM, 0);
  if (socket_fd == -1) {
    printf("socket() failed: %s", strerror(errno));
    return 0;
  }

  // Connect the socket to the remote host.
  int res = connect(socket_fd,
                    (const struct sockaddr *)(addr),
                    addrlen);
  if (res == -1) {
    printf("connect() failed: %s", strerror(errno));
    return 0;
  }

  *ret_fd = socket_fd;
  return 1;
}