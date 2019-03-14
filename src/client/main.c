#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<sys/socket.h>
#include<arpa/inet.h>

#define PORT 5678
#define IPADDR "0.0.0.0"

int main(int argc, char *argv[]) {

  int socket_desc = socket(AF_INET, SOCK_STREAM, 0);

  struct sockaddr_in server;

  char recvmsg[2000], sendmsg[2000];

  // connect to server

  if(socket_desc == -1) {
    printf("ERROR: can't create socket\n");
    return -1;
  }

  server.sin_addr.s_addr = inet_addr(IPADDR);
  server.sin_family = AF_INET;
  server.sin_port = htons(PORT);

  if (
    connect(
      socket_desc,
      (struct sockaddr *)&server,
      sizeof(server)
    ) < 0
  ) {
    printf("ERROR: connection failed\n");
  }

  // set sendmsg
  (argv[2] == NULL) ?
    sprintf(sendmsg, "%s",argv[1]) :
    sprintf(sendmsg, "%s %s", argv[1], argv[2]);

  //printf("%s",sendmsg);

  if (
    send(
      socket_desc,
      sendmsg,
      strlen(sendmsg),
      0
    ) < 0
  ) {
    printf("ERROR: sending failed\n");
  }

  if (
    recv(
      socket_desc,
      recvmsg,
      2000,
      0
    ) < 0
  ) {
    printf("ERROR: receiving failed\n");
  }

  // print answer from server to console
  printf("\n%s\n",recvmsg);

  // close server connection
  close(socket_desc);

  return 0;
}
