#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<signal.h>

#include "server.h"

//   handles input from command line
void *cli(void *a) {

  char cmd[256];
  char peers[BUFSIZE];
  const char whitespace[] = " ";
  regex_t regex;

  // initialize regex for cli
  if (
    regcomp(
      &regex,
      "^CONNECT ([0-9]{1,3}.){3}[0-9]{1,3} [0-9]{1,5}[\t\n\r\f\v]*$",
      REG_EXTENDED
    )
  ) {
    printf("ERROR: compiling regex\n");
    return NULL;
  }

  while(1) {
    printf("> ");
    fflush(stdin);

    gets(cmd);

    if (strcmp(cmd, "quit") == 0) {
      kill(getpid(), 9);
      exit(0);
    } else if (strcmp(cmd, "PEERS") == 0) {
      char x[5000];
      get_peers(x);
      printf("%s",x);
      memset(x,0,sizeof(x));
    } else if (regexec(&regex, cmd, 0, NULL, 0) == 0) {

      char ip[16];
      long port;

      // equals CONNECT
      strtok(cmd, whitespace);
      // equals IP
      strcpy(ip, strtok(NULL, whitespace));
      // equals port
      port = atol(strtok(NULL, whitespace));

      // connect
      connect_peer(ip, port);

    } else {
      printf("INVALID COMMAND!\n");
    }
  }
}

int main(int argc, char *argv[]) {

  long port = (argv[1] == NULL) ?
    DEFAULTPORT :
    atol(argv[1])
  ;

  int socket_description;

  if (init_server(&socket_description, port) == -1)
    return -1;

  pthread_t worker_cli;

  if (
    pthread_create(
      &worker_cli, NULL, cli, (void *)NULL
    ) != 0
  ) {
    printf("ERROR: can't create cli thread\n");
    return -1;
  }

  //int pid_child = fork();

  //if (pid_child == -1) {

  //  printf("ERROR creating child proccess\n");

  //} else if (pid_child == 0) {


  //} else {

    // Parentprocess:
    //   takes incoming requests (the actual server)

  printf("Server listening on: %s:%d\n",IPADDR,port);

  while(1) {
    handle_client(&socket_description);
  }
 // }
}
