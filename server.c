/* Ben Brown
** server.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <ctype.h>
#include <stdio.h>
#include "string.h"
#include "stdlib.h"
#include "server.h"

#define BACKLOG 10     // how many pending connections queue will hold

void sigchld_handler(int s)
{
  // waitpid() might overwrite errno, so we save and restore it:
  int saved_errno = errno;

  while(waitpid(-1, NULL, WNOHANG) > 0);

  errno = saved_errno;
};

// IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET)
  {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
};

int main(int argc, char * argv[])
{
  int sockfd;                             // listen on sock_fd
  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_storage their_addr;     // connector's address information
  socklen_t sin_size;
  struct sigaction sa;
  int yes=1;
  char s[INET6_ADDRSTRLEN];
  int rv;
  int connfd = 0;
  int sizeName = 0;
  char buff[5];
  FILE *out_file;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE; // use my IP

  if (argc != 2)
  {
      fprintf(stderr,"usage: <port>\n");
      exit(1);
  }
  if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0)
  {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
      return 1;
  }

  // loop through all the results and bind to the first we can
  for(p = servinfo; p != NULL; p = p->ai_next)
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
    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sockfd);
      perror("server: bind");
      continue;
    }
    break;
  }

  freeaddrinfo(servinfo); // temporary structure, don't need it anymore

  if (p == NULL)
  {
    fprintf(stderr, "server: failed to bind\n");
    exit(1);
  }
  if (listen(sockfd, BACKLOG) == -1) {
    perror("listen");
    exit(1);
  }

  printf("server: waiting for connections...\n");

  connfd = accept (sockfd, (struct sockaddr*)NULL, NULL);
  if (connfd == -1)
    perror("accept");

  inet_ntop(their_addr.ss_family,get_in_addr((struct sockaddr *)&their_addr),s, sizeof s);
  printf("server: got connection from %s\n", s);

  //At this point connection has been established with the client and the server
  //can begin to read in from the client.

  // get the file size
  int size = readFileSize(connfd,buff);

  printf("File Name Size: %d\n",size);

  //create char array big enough to hold the file size
  char * fileName = (char *) malloc (size);

  // read the output file name
  char * temp = fileName;

  start(connfd,buff);

  readFile(connfd,buff,temp);

  printf("File Name: %s\n",fileName);

  if( access(fileName, F_OK) != -1 )
  {
    printf("file already exists!\n");
    return 0;
  }

  //open file
  out_file = fopen(fileName,"w"); // check to see if file already exist and return error if it does and write to client


  //loops until != 0
  start(connfd,buff);
  readContent(connfd,buff,out_file);

  printf("Successfully wrote to %s!\n",fileName);

  close (connfd);
  return 0;
}


//Function implementation

void start(int connfd, char * buff)
{
  int i;
  read(connfd,buff,5);
  //not infinite loop to account for an empty text file
  for (i = 0; i < 10; i++)
    if(buff[0] == 0)
      read(connfd,buff,5);
}

int readFileSize(int connfd, char * buff)
{
  int size = 0;
  read(connfd,buff,5);

  while(strlen(buff) == 5)
  {
    size+=5;
    read(connfd,buff,5);
  }

  int i = strlen(buff);
  size+=i;

  return size;
}


void readFile(int connfd, char * buff, char * temp)
{
  int i = 0;
  int mod;
  while (buff[0] != 0)
  {
    mod = i%2;
    if (mod == 0)
      strncpy(temp,&buff[1],4);
    else
      strncpy(temp,buff,5);
    read(connfd,buff,5);
    if (mod == 0)
      temp+=4;
    else
      temp+=5;
      i++;
  }
}

void readContent(int connfd, char * buff, FILE * out_file)
{
  int i = 0;
  int mod;

  while (buff[0] != 0)
  {
    mod = i%2;
    if (buff[4] == 0)
    {
      if (mod == 0)
        fwrite(&buff[1],1,strlen(&buff[1]),out_file);
      else
        fwrite(buff,1,strlen(&buff[1]),out_file);
    }
    else if (mod == 0)
      fwrite(&buff[1],1,4,out_file);
    else
      fwrite(buff,1,5,out_file);
    read(connfd,buff,5);
    i++;
  }
}
