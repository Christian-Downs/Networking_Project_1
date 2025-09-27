/*
 * CS447 P1 SERVER STARTER CODE
 * ----------------------------
 *  Author: Thoshitha Gamage
 *  Date:   09/08/2025
 *  Licence: MIT Licence
 *  Description: This is the starter code for CS447 Fall 2025 P1 server.This code is based on the simple stream server code
 *      found on Beej's Guide to Network programming at https://beej.us/guide/bgnet/html/#a-simple-stream-server.
 *      The code was adapted to use C++20 features like std::jthread for concurrency.
 *
 *      This code can be compiled using:
 *           g++ -std=c++20 -Wall -pthread server.cpp -o server
 *
 *      Use this code as the base for your server implmentation.
 *
 */

#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#include <system_error>

// C headers for socket API
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include "p1_helper.h"

#define PORT "3490"
#define BACKLOG 10
#define MAXDATASIZE 100

// Helper function to get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET)
  {
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

// Function to handle a single client connection in its own thread
void handle_client(int new_fd, struct sockaddr_storage their_addr)
{
  // A temporary buffer for the client's IP address string
  char s[INET6_ADDRSTRLEN];
  inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
  std::cout << "server: got connection from " << s << std::endl;
  int numbytes;

  // Send the message to the client
  const char *msg = "Hello, world!";
  if (send(new_fd, msg, strlen(msg), 0) == -1)
  {
    perror("send");
  }

  char buf[MAXDATASIZE];
  for (;;)
  {
    int numbytes = recv(new_fd, buf, MAXDATASIZE - 1, 0);
    if (numbytes == 0)
    {
      // client closed
      break;
    }
    if (numbytes < 0)
    {
      perror("recv");
      break;
    }
    buf[numbytes] = '\0';
    buf[strcspn(buf, "\r\n")] = '\0'; // Strip newline characters
    if(strcmp(buf, "BYE") == 0) {
      printf("server: received BYE, closing connection\n");
      break;
    }
    printf("server: received '%s'\n", buf);

    // optional echo back
    // if (send(new_fd, buf, numbytes, 0) == -1) perror("send");
  }

  // Close the socket for this connection
  close(new_fd);
  std::cout << "server: connection with " << s << " closed." << std::endl;
}

int main()
{
  int sockfd, new_fd;
  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_storage their_addr;
  socklen_t sin_size;
  int yes = 1;
  int rv;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0)
  {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }

  for (p = servinfo; p != NULL; p = p->ai_next)
  {
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
    {
      perror("server: socket");
      continue;
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
    {
      perror("setsockopt");
      exit(1);
    }

    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
    {
      close(sockfd);
      perror("server: bind");
      continue;
    }
    break;
  }
  freeaddrinfo(servinfo);

  if (p == NULL)
  {
    fprintf(stderr, "server: failed to bind\n");
    exit(1);
  }
  if (listen(sockfd, BACKLOG) == -1)
  {
    perror("listen");
    exit(1);
  }
  std::cout << "server: waiting for connections..." << std::endl;

  while (true)
  {
    sin_size = sizeof their_addr;
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);

    if (new_fd == -1)
    {
      perror("accept");
      continue;
    }

    // Create a new thread to handle the accepted connection
    // std::jthread automatically joins upon destruction
    std::jthread(handle_client, new_fd, their_addr);
  }

  // The main loop will never exit, so this is unreachable.
  close(sockfd);
  return 0;
}
