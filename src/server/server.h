#include<stdio.h>
#include<stdlib.h>
#include<ctype.h>
#include<string.h>
#include<unistd.h>
#include<regex.h>
#include<sys/shm.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<pthread.h>
#include<semaphore.h>
#include<float.h>

#include "../sensors.h"

#define QUEUESIZE        10000
#define DEFAULTPORT       5678
#define IPADDR       "0.0.0.0"
#define WORKERCOUNT        100
#define SENSORCOUNT          7
#define APICOUNT             4
#define BUFSIZE           4000
#define SENDTIMEOUT         10
#define RECOGNIZORTIMEOUT   10
#define SDCOLSIZE           24
#define IPCOLSIZE           16
#define ADAPTERSIZE          3

typedef struct {
  double minimum;
  double maximum;
  double last;
} SensorData;

typedef struct {
  char       ip[16];
  int        sock;
  SensorData data[SENSORCOUNT];
} Client;

int connect_peer(char *ip, long port);
int init_server(int *socket_description, long port);
void handle_client(int *socket_description);
void get_peers(char *buf);
