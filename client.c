/* Ben Brown
** client.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <ctype.h>
#include "string.h"
#include "stdlib.h"
#include "client.h"

// max number of bytes
#define MAXDATASIZE 100 

// get sockaddr, IPv4 or IPv6
void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET)
  {
      return &(((struct sockaddr_in*)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
  int sockfd, numbytes;
  char buf[MAXDATASIZE];
  struct addrinfo hints, *servinfo, *p;
  int rv;
  char s[INET6_ADDRSTRLEN];
  char buff[10];
  char fileSize[100];
  FILE *in_file;

  if (argc != 5)
  {
    fprintf(stderr,"usage: <srcfile> <destfile> <desthost> <destport>\n");
    exit(1);
  }

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if ((rv = getaddrinfo(argv[3], argv[4], &hints, &servinfo)) != 0)
  {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }

  // loop through all the results and connect to the first we can
  for(p = servinfo; p != NULL; p = p->ai_next)
  {
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
    {
      perror("client: socket");
      continue;
    }
    if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
    {
      close(sockfd);
      perror("client: connect");
      continue;
    }
    break;
  }

  if (p == NULL)
  {
    fprintf(stderr, "client: failed to connect\n");
    return 2;
  }

  inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
  printf("client: connecting to %s\n", s);

  freeaddrinfo(servinfo); 

  int input_size = strlen(argv[2]);
  int name_size = input_size+1;

  //hold the name of the input file in a temporary buff
  char * name = (char *) malloc (name_size);
  strcpy(name, argv[2]);

  //Send over the file size
  sendFileSize(sockfd,buff,name_size);

  // Send over the name of the input file
  char * temp = name;

  //!0 is write
  //0 is phase shift- dont write packet
  //server will loop until control byte != 0

  //set to write
  memset(buff,-1,1);
  memset(&buff[9],-1,1);

  sendFile(sockfd,buff,temp);

  //write a phase change packet in case buff is greater than 5 but less than 10
  memset(buff,0,10);
  write(sockfd,buff,10);

  in_file = fopen (argv[1],"r");
  if (in_file == NULL) {fputs ("File error",stderr); exit (1);}

  //send over the contents of the file

  //set to write
  memset(buff,-1,1);
  memset(&buff[9],-1,1);

  sendContent(sockfd,buff,in_file);

  //write a phase change packet in case buff is greater than 5 but less than 10
  memset(buff,0,10);
  write(sockfd,buff,10);

  close(sockfd);
  return 0;
}



//Function implementations
void sendFileSize(int sockfd, char * buff, int name_size)
{
  int part = name_size/10;
  int mod = name_size%10;

  printf("part: %d", part);
  printf("mod: %d", mod);

  int i;
  memset(buff,-1,10);
  for (i = 0; i < part; i++)     //buff of ten -1s is written
  {
    write(sockfd,buff,10);
  }
  memset(buff,0,10);
  memset(buff,-1,mod);           //fills buff with the remaining size of the filename
  write(sockfd,buff,10);

                                 //write a phase packet in case buff is greater than 5 but less than 10
  memset(buff,0,10);
  write(sockfd,buff,10);
}

// send over the file name
void sendFile(int sockfd, char * buff, char * temp)
{
  while(buff[9] != 0)           //stop case indicates the end of the filename
  {
    memset(&buff[1],0,9);       //clears out buff
    strncpy(&buff[1],temp,9);   //copys over the right portion from name to buff
    if (buff[1] == 0)           //indicates that previous write was the end of the file
      memset(buff,0,1);
    write(sockfd,buff,10);
    temp+=9;                    //increment through the filename the same amount that was just written
  }
}

//send over the contents of the file
void sendContent(int sockfd, char * buff, FILE * in_file)
{
  while(buff[9] != 0)
  {
    memset(&buff[1],0,9);
    fread(&buff[1],1,9,in_file);
    if (buff[1] == 0)
      memset(buff,0,1);
    write(sockfd,buff,10);
  }
}
