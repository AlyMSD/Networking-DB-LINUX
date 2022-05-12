#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include "msg.h"
#include <fcntl.h>

struct clientSocket {
  int c_fd;
  struct sockaddr *addr;
  size_t addrlen;
  int sock_family;
};

void PrintOut(int fd, struct sockaddr *addr, size_t addrlen);
void PrintReverseDNS(struct sockaddr *addr, size_t addrlen);
void PrintServerSide(int client_fd, int sock_family);

void Usage(char *progname);
int  Listen(char *portnum, int *sock_family);
void* HandleClient(void* c_data);
// initialize to lock threads
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int
main(int argc, char **argv) {
//file
  FILE *fp;
  // Expect the port number as a command line argument.
  if (argc != 2) {
    Usage(argv[0]);
  }

  int sock_family;
  int listen_fd = Listen(argv[1], &sock_family);
  if (listen_fd <= 0) {
    // We failed to bind/listen to a socket.  Quit with failure.
    printf("Couldn't bind to any addresses.\n");
    return EXIT_FAILURE;
  }
  
   // create file if not already present
  fp = fopen ("data", "ab+");
      if (fp == NULL)
      {
        fprintf(stderr, "\nError opened file\n");
        exit (1);
      }
      // close file after creation
  fclose (fp);
  

  // Loop forever, accepting a connection from a client and doing
  // an echo trick to it.
  while (1) {
    struct sockaddr_storage caddr;
    socklen_t caddr_len = sizeof(caddr);
    int client_fd = accept(listen_fd,
                           (struct sockaddr *)(&caddr),
                           &caddr_len);
    if (client_fd < 0) {
      if ((errno == EINTR) || (errno == EAGAIN) || (errno == EWOULDBLOCK))
        continue;
      printf("Failure on accept:%s \n ", strerror(errno));
      break;
    }
   
     // struct to pass socket data to thread
    struct clientSocket* clientData = malloc(sizeof(*clientData));
    clientData->c_fd = client_fd;
    clientData->addr = (struct sockaddr *)(&caddr);
    clientData->addrlen = caddr_len;
    clientData->sock_family = sock_family;
    // create thread
    pthread_t thread;
    pthread_create(&thread, NULL, HandleClient, clientData);
  }
  
  return EXIT_SUCCESS;
}



void *
HandleClient(void* clientData) {
  FILE *fp;
  // Print out information about the client.
  struct clientSocket c_data = *((struct clientSocket*)clientData);
  printf("\nNew client connection \n" );
  PrintOut(c_data.c_fd, c_data.addr, c_data.addrlen);
  PrintReverseDNS(c_data.addr, c_data.addrlen);
  PrintServerSide(c_data.c_fd, c_data.sock_family);
  
  // var for struct data used
  struct msg ms;
  ms.type = -1;
    //char clientbuf[1024];
    // Loop, reading data and echo'ing it back, until the client
  // closes the connection.
    while(ms.type != 0){
    // read data from client
    read(c_data.c_fd, &ms, sizeof(ms));
     // if client exit
    if (ms.type == 0) {
      printf("[The client disconnected.] \n");
      //break;
    }
    // if put
    else if (ms.type == 1)
    {
      //open file to append
      fp = fopen ("data", "a");
      if (fp == NULL)
      {
        fprintf(stderr, "\nError opened file\n");
        exit (1);
      }
       // write to file
      fseek(fp, 0, SEEK_SET);
      fwrite (&ms.rd, sizeof(struct record), 1, fp);
      fclose (fp);
      // send success
      write(c_data.c_fd, "Put Success", sizeof("Put Success\n"));
    }
    // if get
    else if (ms.type == 2)
    {
        //var to check if data found for fail message
      int found = -1;
      struct msg temp;
      // open to read file
      fp = fopen ("data", "rb");
      if (fp == NULL)
      {
        fprintf(stderr, "\nError opened file\n");
        exit (1);
      }
      // while not end of file read data
      while(fread(&temp.rd, sizeof(struct record), 1, fp))
      { 
        // if data is same as user
        if(temp.rd.id == ms.rd.id)
        {
          // found = 1
          found = 1;
          // send data to client
          write(c_data.c_fd, &temp, sizeof(temp));
        }
      }
      fclose (fp);
      // if data not found
      if (found == -1)
      {
        temp.rd.id = -99999;
        write(c_data.c_fd, &temp, sizeof(temp));
      }
    }
    // if delete
    else if (ms.type == 3)
    {
    //var to check if data found for fail message
      int found = -1;
      // temp structs to pass data and modify data in file
      struct msg temp;
      struct msg temp2;
      //when data is delted mark as delted with negative val for id
      temp2.rd.id = -1111;
      // open file to read and modify
      fp = fopen ("data", "rb+");
      if (fp == NULL)
      {
        fprintf(stderr, "\nError opened file\n");
        exit (1);
      }
      // while not end of file
      while(fread(&temp.rd, sizeof(struct record), 1, fp))
      {
        // if data is same as client
        if(temp.rd.id == ms.rd.id)
        {
          //found = 1
          found = 1;
          // send data to user
          write(c_data.c_fd, &temp, sizeof(temp));
          // modify data to mark as deleted
          fseek(fp, -sizeof(temp.rd), SEEK_CUR);
          fwrite (&temp2.rd, sizeof(struct record), 1, fp);
          
        }
      }
      // if not found tell user failed
      if (found == -1)
      {
        // data not found modify temp to signify not found
        temp.rd.id = -99999;
        write(c_data.c_fd, &temp, sizeof(temp));
      }
      //close file
      fclose (fp);
    }
  }
    // Really should do this in a loop in case of EAGAIN, EINTR,
    // or short write, but I'm lazy.  Don't be like me. ;)
    //write(c_data.c_fd, "You typed: ", strlen("You typed: "));
    //write(c_data.c_fd, clientbuf, strlen(clientbuf));
  close(c_data.c_fd);
  return NULL;
}







int
Listen(char *portnum, int *sock_family) {

  // Populate the "hints" addrinfo structure for getaddrinfo().
  // ("man addrinfo")
  struct addrinfo hints;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;       // IPv6 (also handles IPv4 clients)
  hints.ai_socktype = SOCK_STREAM;  // stream
  hints.ai_flags = AI_PASSIVE;      // use wildcard "in6addr_any" address
  hints.ai_flags |= AI_V4MAPPED;    // use v4-mapped v6 if no v6 found
  hints.ai_protocol = IPPROTO_TCP;  // tcp protocol
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;

  // Use argv[1] as the string representation of our portnumber to
  // pass in to getaddrinfo().  getaddrinfo() returns a list of
  // address structures via the output parameter "result".
  struct addrinfo *result;
  int res = getaddrinfo(NULL, portnum, &hints, &result);

  // Did addrinfo() fail?
  if (res != 0) {
printf( "getaddrinfo failed: %s", gai_strerror(res));
    return -1;
  }

  // Loop through the returned address structures until we are able
  // to create a socket and bind to one.  The address structures are
  // linked in a list through the "ai_next" field of result.
  int listen_fd = -1;
  struct addrinfo *rp;
  for (rp = result; rp != NULL; rp = rp->ai_next) {
    listen_fd = socket(rp->ai_family,
                       rp->ai_socktype,
                       rp->ai_protocol);
    if (listen_fd == -1) {
      // Creating this socket failed.  So, loop to the next returned
      // result and try again.
      printf("socket() failed:%s \n ", strerror(errno));
      listen_fd = -1;
      continue;
    }

    // Configure the socket; we're setting a socket "option."  In
    // particular, we set "SO_REUSEADDR", which tells the TCP stack
    // so make the port we bind to available again as soon as we
    // exit, rather than waiting for a few tens of seconds to recycle it.
    int optval = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR,
               &optval, sizeof(optval));

    // Try binding the socket to the address and port number returned
    // by getaddrinfo().
    if (bind(listen_fd, rp->ai_addr, rp->ai_addrlen) == 0) {
      // Bind worked!  Print out the information about what
      // we bound to.
      PrintOut(listen_fd, rp->ai_addr, rp->ai_addrlen);

      // Return to the caller the address family.
      *sock_family = rp->ai_family;
      break;
    }

    // The bind failed.  Close the socket, then loop back around and
    // try the next address/port returned by getaddrinfo().
    close(listen_fd);
    listen_fd = -1;
  }

  // Free the structure returned by getaddrinfo().
  freeaddrinfo(result);

  // If we failed to bind, return failure.
  if (listen_fd == -1)
    return listen_fd;

  // Success. Tell the OS that we want this to be a listening socket.
  if (listen(listen_fd, SOMAXCONN) != 0) {
    printf("Failed to mark socket as listening:%s \n ", strerror(errno));
    close(listen_fd);
    return -1;
  }

  // Return to the client the listening file descriptor.
  return listen_fd;
}








void
PrintOut(int fd, struct sockaddr *addr, size_t addrlen) {
  printf("Socket [%d] is bound to: \n", fd);
  if (addr->sa_family == AF_INET) {
    // Print out the IPV4 address and port

    char astring[INET_ADDRSTRLEN];
    struct sockaddr_in *in4 = (struct sockaddr_in *)(addr);
    inet_ntop(AF_INET, &(in4->sin_addr), astring, INET_ADDRSTRLEN);
    printf(" IPv4 address %s", astring);
    printf(" and port %d\n", ntohs(in4->sin_port));

  } else if (addr->sa_family == AF_INET6) {
    // Print out the IPV6 address and port

    char astring[INET6_ADDRSTRLEN];
    struct sockaddr_in6 *in6 = (struct sockaddr_in6 *)(addr);
    inet_ntop(AF_INET6, &(in6->sin6_addr), astring, INET6_ADDRSTRLEN);
    printf("IPv6 address %s", astring);
    printf(" and port %d\n", ntohs(in6->sin6_port));

  } else {
    printf(" ???? address and port ???? \n");
  }
}

void
PrintReverseDNS(struct sockaddr *addr, size_t addrlen) {
  char hostname[1024];  // ought to be big enough.
  if (getnameinfo(addr, addrlen, hostname, 1024, NULL, 0, 0) != 0) {
    sprintf(hostname, "[reverse DNS failed]");
  }
  printf("DNS name: %s \n", hostname);
}

void
PrintServerSide(int client_fd, int sock_family) {
  char hname[1024];
  hname[0] = '\0';

  printf("Server side interface is ");
  if (sock_family == AF_INET) {
    // The server is using an IPv4 address.
    struct sockaddr_in srvr;
    socklen_t srvrlen = sizeof(srvr);
    char addrbuf[INET_ADDRSTRLEN];
    getsockname(client_fd, (struct sockaddr *) &srvr, &srvrlen);
    inet_ntop(AF_INET, &srvr.sin_addr, addrbuf, INET_ADDRSTRLEN);
    printf("%s", addrbuf);
    // Get the server's dns name, or return it's IP address as
    // a substitute if the dns lookup fails.
    getnameinfo((const struct sockaddr *) &srvr,
                srvrlen, hname, 1024, NULL, 0, 0);
    printf(" [%s]\n", hname);
  } else {
    // The server is using an IPv6 address.
    struct sockaddr_in6 srvr;
    socklen_t srvrlen = sizeof(srvr);
    char addrbuf[INET6_ADDRSTRLEN];
    getsockname(client_fd, (struct sockaddr *) &srvr, &srvrlen);
    inet_ntop(AF_INET6, &srvr.sin6_addr, addrbuf, INET6_ADDRSTRLEN);
    printf("%s", addrbuf);
    // Get the server's dns name, or return it's IP address as
    // a substitute if the dns lookup fails.
    getnameinfo((const struct sockaddr *) &srvr,
                srvrlen, hname, 1024, NULL, 0, 0);
    printf(" [%s]\n", hname);
  }
}


void Usage(char *progname) {
  printf("usage: %s port \n", progname);
  exit(EXIT_FAILURE);
}