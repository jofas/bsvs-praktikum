#include "server.h"
#include "buffer.h"

// every possible sensor a peer
// could have implemented.
const char *SENSORS[] = {
  "TEMPERATURE",
  "HUMIDITY",
  "LIGHT",
  "DISTANCE",
  "SOUND",
  "ANGLE",
  "MOISTURE"
};

// Queue in which the clients requests
// that are waiting to be handled are stored.
Client task_queue[QUEUESIZE];

// Queue containing connected clients.
// For each index there are two working threads,
// worker send und worker receive.
Client cur_clients[WORKERCOUNT];

// mutexes and conditions for cur_clients
pthread_mutex_t m_cur_clients[WORKERCOUNT];
//
pthread_cond_t c_cur_clients[WORKERCOUNT];
//
// execution state of worker send
int current_sensor[WORKERCOUNT];

// array containing the ids of the worker threads
// (just an array that equals [0, 1, 2, ..., WORKERCOUNT-1],
// but since we need a static place (to avoid dangling
// pointers) for this we save it as an array)
int worker_id[WORKERCOUNT];

int recognizor_id[APICOUNT];

// API described at:
//
// [
//   pubs.opengroup.org/onlinepubs/7908799/xns/arpainet.h.html,
//   pubs.opengroup.org/onlinepubs/7908799/xns/netinetin.h.html
// ]
//
// arpa::inet / netinet::in defines sockaddr_in
// which can be used to store ip addresses.
//
// (!) If used with sys::socket values must be
//     cast to sys::socket::sockaddr.
//
typedef struct sockaddr_in SockAddr;

// index for where to put or take next client request
int next_put = 0, next_take = 0;

sem_t sem_put;
sem_t sem_take;

// mutex for creating safe access to indexes
pthread_mutex_t m_next_put, m_next_take;

// mutex for creating safe access to next_put
pthread_mutex_t m_next_put;

// regex for incoming data from other servers
regex_t r_sensorresp, r_recognitionresp, r_ip;
pthread_mutex_t m_sensorresp, m_recognitionresp, m_ip;

// safe concurrent put function that
// puts the incoming socket to
// task_queue. Secured with Semaphores
// that register how many free slots
// task_queue has.
void put(Client c) {

  // if task_queue is full this thread has to wait.
  sem_wait(&sem_put);

  pthread_mutex_lock(&m_next_put);

  // put client_sock in task_queue
  memcpy(
    (void *)&task_queue[next_put],
    (void *)&c,
    sizeof(Client)
  );

  next_put = next_put == QUEUESIZE - 1 ?
    0 : next_put + 1;

  pthread_mutex_unlock(&m_next_put);

  // client_sock is in task_queue -> "take++"
  sem_post(&sem_take);
}

// safe concurrent take function that
// takes the next socket from task_queue
// (which is the one waiting the longest).
// Secured with Semaphores that register
// how many free slots task_queue has and
// a mutex for manipulating the index that
// points to the next socket.
Client take() {
  Client c;

  // safe area in which the variable next_take
  // can be accessed.
  {
    pthread_mutex_lock(&m_next_take);

    c = task_queue[next_take];

    task_queue[next_take].sock = -2;

    next_take = next_take == QUEUESIZE - 1 ?
      0 : next_take + 1;

    pthread_mutex_unlock(&m_next_take);
  }

  return c;
}

// higher order iterator over cur_clients
void cur_clients_for_each(void *f(Client *c, void *a), void *a) {
  for(int i = 0; i < WORKERCOUNT; i++) {
    pthread_mutex_lock(&m_cur_clients[i]);
    if ( cur_clients[i].sock != -2 ) f(&cur_clients[i], a);
    pthread_mutex_unlock(&m_cur_clients[i]);
  }
}

// entry for each peer -> called in get_peers
void* peer_entry(Client *c, void *a) {
  char *buf = (char *)a;

  // ip
  strcat(buf, c->ip);
  fill_col(buf,strlen(c->ip), IPCOLSIZE);

  char data[9]; // SDCOLSIZE/3 + 1 (wegen \0)

  // print sensordata
  for (int j = 0; j < SENSORCOUNT; j++) {

    // min
    if (c->data[j].minimum == DBL_MIN) {
      sprintf(data, "void");
    } else {
      sprintf(data, "%.2lf ", c->data[j].minimum);
    }
    fill_col(data,strlen(data), SDCOLSIZE/3);
    strcat(buf, data);

    // last
    if (c->data[j].last == DBL_MIN) {
      sprintf(data, "void");
    } else {
      sprintf(data, "%.2lf ", c->data[j].last);
    }
    fill_col(data,strlen(data), SDCOLSIZE/3);
    strcat(buf, data);

    // max
    if (c->data[j].maximum == DBL_MIN) {
      sprintf(data, "void");
    } else {
      sprintf(data, "%.2lf ", c->data[j].maximum);
    }
    fill_col(data,strlen(data), SDCOLSIZE/3);
    strcat(buf, data);
  }
  strcat(buf,"\n");
}

// function that returns the connected peers
// (clients in cur_clients)
void get_peers(char *buf) {

  // Headerline
  {
    strcat(buf, "IP");
    fill_col(buf,2, IPCOLSIZE);

    for (int i = 0; i < SENSORCOUNT; i++) {
      strcat(buf, SENSORS[i]);
      fill_col(buf,strlen(SENSORS[i]), SDCOLSIZE);
    }
    strcat(buf, "\n");
  }

  // Subheaderline
  {
    fill_col(buf,0,IPCOLSIZE);
    for (int i = 0; i < SENSORCOUNT; i++) {
      strcat(buf, "MIN     ");
      strcat(buf, "LAST    ");
      strcat(buf, "MAX     ");
    }
    strcat(buf, "\n");
  }

  // Entry for each connected Client
  cur_clients_for_each(peer_entry, (void *)buf);
}

// API our server provides
void generate_response(char *buf_in, char *buf_out) {
  if (strcmp(buf_in, "GET TEMPERATURE")     == 0) {
    // temperature in Celcius
    sprintf(buf_out,"Temperature: %.02f",get_temperature());
  } else if (strcmp(buf_in, "GET HUMIDITY") == 0) {
    // humidity in %
    sprintf(buf_out,"Humidity: %.02f",get_humidity());
  } else if (strcmp(buf_in, "GET DISTANCE") == 0) {
    // distance in cm
    sprintf(buf_out,"Distance: %d",get_distance());
  } else if (strcmp(buf_in, "GET LIGHT")    == 0) {
    // light in ???
    sprintf(buf_out,"Light: %d",get_light());
  } else if (strcmp(buf_in, "PEERS")        == 0) {
    get_peers(buf_out);
  } else {
    sprintf(buf_out, "void\n");
  }
}

// worker receive which is basically a
// thread running an endless loop looking
// for sockets in the task_queue.
// Worker receive handles the receiving and
// responding from clients.
void* work_rcv(void* a) {

  int id = *(int *)a;
  char recvmsg[BUFSIZE], sendmsg[BUFSIZE];

  if (id < 0 || id >= WORKERCOUNT) {
    printf("ERROR: WORKER RECEIVE WITH INVALID ID: %d\n", id);
    return NULL;
  }

  while(1) {

    // if task_queue is empty this thread has to wait.
    sem_wait(&sem_take);
    // safe area in which cur_clients[id]
    // can be accessed (PEERS and worker send
    // access it too).
    pthread_mutex_lock(&m_cur_clients[id]);
    // move waiting client to a connected one
    cur_clients[id] = take();

    //printf(
    //  "Worker: %d took client: %s on socket %d\n",
    //  id,
    //  cur_clients[id].ip,
    //  cur_clients[id].sock
    //);

    pthread_mutex_unlock(&m_cur_clients[id]);
    // waiting client taken out of task_queue -> "put++"
    sem_post(&sem_put);

    // sys::socket::recv
    while(recv(cur_clients[id].sock,recvmsg,BUFSIZE,0) > 0) {

      //printf("Client %s sends: %s\n", cur_clients[id].ip,recvmsg);

      to_upper(recvmsg);

      // check if incoming data is sensor data
      pthread_mutex_lock(&m_sensorresp);
      int no_match = regexec(&r_sensorresp, recvmsg, 0, NULL, 0);
      pthread_mutex_unlock(&m_sensorresp);

      // the received data is valid sensor data ->
      // save sensor data in client
      if (!no_match) {
        pthread_mutex_lock(&m_cur_clients[id]);
        double sensordata = parse_sensor_data(recvmsg);
        cur_clients[id].data[current_sensor[id]].last = sensordata;

        if (
          sensordata > cur_clients[id].data[current_sensor[id]].maximum ||
          cur_clients[id].data[current_sensor[id]].maximum == DBL_MIN
        ) cur_clients[id].data[current_sensor[id]].maximum = sensordata;
        if (
          sensordata < cur_clients[id].data[current_sensor[id]].minimum ||
          cur_clients[id].data[current_sensor[id]].minimum == DBL_MIN
        ) cur_clients[id].data[current_sensor[id]].minimum = sensordata;

        // valid data came from client and was handled ->
        // wake up sender
        pthread_cond_signal(&c_cur_clients[id]);
        pthread_mutex_unlock(&m_cur_clients[id]);

        memset(&recvmsg[0],0,sizeof(recvmsg));
        continue;
      }

      pthread_mutex_lock(&m_recognitionresp);
      no_match = regexec(&r_recognitionresp, recvmsg, 0, NULL, 0);
      pthread_mutex_unlock(&m_recognitionresp);

      // the received data is a recognition ->
      // print it to CLI
      if (!no_match) {
        pthread_mutex_lock(&m_ip);
        no_match = regexec(&r_ip, recvmsg, 0, NULL, 0);
        pthread_mutex_unlock(&m_ip);

        if(!no_match) {
          // the ip was submitted
          printf("NODE %s\n",recvmsg);
        } else {
          // the ip was not submitted ->
          // take it from client object
          printf("NODE %s %s\n",cur_clients[id].ip, recvmsg);
        }

        memset(&recvmsg[0],0,sizeof(recvmsg));
        continue;
      }

      // the input is neither sensor data nor recogintion ->
      // give the request to our API

      // generate response data from sensors
      generate_response(recvmsg, sendmsg);
      // unistd::write
      write(cur_clients[id].sock,sendmsg,strlen(sendmsg));
      //printf("Worker %d answered: %s\n",id,sendmsg);
      memset(&sendmsg[0],0,sizeof(sendmsg));
      memset(&recvmsg[0],0,sizeof(recvmsg));
    }

    pthread_mutex_lock(&m_cur_clients[id]);
    // connection lost -> reset the client
    // handled by this worker
    //printf(
    //  "Worker: %d lost connection to: %s\n",
    //  id,
    //  cur_clients[id].ip
    //);
    cur_clients[id].sock = - 2;
    pthread_mutex_unlock(&m_cur_clients[id]);
  }
}

// worker send which is basically a thread
// running an endless loop looking for
// already connected sockets in cur_clients.
// If the socket is valid this thread sends
// requests to the socket every 10 seconds.
void* work_snd(void* a) {

  int id = *(int *)a;
  char recvmsg[BUFSIZE], sendmsg[BUFSIZE];

  if (id < 0 || id >= WORKERCOUNT) {
    printf("ERROR: WORKER SEND WITH INVALID ID: %d\n", id);
    return NULL;
  }

  while(1) {

    // wait 10 seconds
    sleep(SENDTIMEOUT);

    // safe area in which cur_clients[id]
    // can be accessed. (PEERS and worker receive
    // access it also)
    pthread_mutex_lock(&m_cur_clients[id]);

    // send requests for sensor information
    for (int i = 0; i < SENSORCOUNT; i++) {

      // check if cur_clients[id] is a valid connection
      if ( cur_clients[id].sock != -2 ) {

        // safe state in current_sensor[id]
        current_sensor[id] = i;

        sprintf(sendmsg, "GET %s",SENSORS[i]);

        //printf("Worker %d asks: %s\n",id, sendmsg);

        // unistd::write
        write(cur_clients[id].sock,sendmsg,strlen(sendmsg));

        // reset sendmsg
        memset(&sendmsg[0],0,sizeof(sendmsg));

        // lock until reciever worker has response
        pthread_cond_wait(&c_cur_clients[id], &m_cur_clients[id]);

      } else {
        // invalid socket
        break;
      }
    }
    pthread_mutex_unlock(&m_cur_clients[id]);
  }
}

// A 2.2 Push-Sensor Events
// {{{

enum { temperature, humidity, distance, light };
enum { green, yellow, red };

void *broadcast(Client *c, void *a) {
  char *sendmsg = (char *)a;
  write(c->sock,sendmsg,strlen(sendmsg));
}

void *recognizor(void *a) {
  int api = *(int *)a;
  int priority;

  //printf("RECOGNIZOR: %d\n",api);

  float latest = FLT_MIN;
  float new;

  while(1) {
    sleep(RECOGNIZORTIMEOUT);

    char sendmsg[BUFSIZE];

    switch (api) {
      case temperature : new = get_temperature();     priority = green;  break;
      case humidity    : new = get_humidity();        priority = green;  break;
      case distance    : new = (float)get_distance(); priority = yellow;  break;
      case light       : new = (float)get_light();    priority = green; break;
      default          : printf("ERROR wrong API number\n");
    }

    if( new != latest && latest != FLT_MIN ) {
      switch (api) {
        case temperature : sprintf(sendmsg, "RECOGNIZED TEMPERATURECHANGED");break;
        case humidity    : sprintf(sendmsg, "RECOGNIZED HUMIDITYCHANGED");   break;
        case distance    : sprintf(sendmsg, "RECOGNIZED MOTION");            break;
        case light       : sprintf(sendmsg, "RECOGNIZED LIGHTCHANGED");      break;
        default          : printf("ERROR wrong API number\n"); return NULL;
      }
      cur_clients_for_each(broadcast, (void *)sendmsg);
      // set lcd display
      // (!) SPAMS STDOUT WITH ERRORS
      //set_lcd(sendmsg, priority);
    }
    latest = new;
  }
}

// }}}

// A 2.1 CONNECT COMMAND
// {{{

// global variable used for checking if peer
// is already connected
int conn_exists = 0;

void *check_for_ip(Client *c, void *a) {
  char *ip = (char *)a;

  if(strcmp(ip, c->ip) == 0) {
    conn_exists = 1;
  }
}

void find_ips(char *my_ip, char *adapter) {
  FILE *ce;
  char cmd[BUFSIZE];

  sprintf(
    cmd,
    "ip address show %s | grep inet[^6] | awk '{print substr($2,0,(length($2)-2))}'",
    adapter
  );

  ce = popen(cmd, "r");
  if (ce == NULL) {
    printf("ERROR: can't find ip address\n");
  }

  fgets(my_ip, sizeof(char)*17, ce);
  // cut trailing newline character
  my_ip = strtok(my_ip, "\n");
}

enum { lo, eth0, wlan0 };

// pub function for connecting to other host
int connect_peer(char *ip, long port) {

  // we don't want to connect to ourself, so we
  // check if the ip is one of ours, if so the
  // connection is refused.
  {
    char my_ips[3][17];

    sprintf(my_ips[lo], "127.0.0.1");
    find_ips(my_ips[eth0], "eth0");
    find_ips(my_ips[wlan0], "wlan0");

    for (int i = 0; i < ADAPTERSIZE; i++) {
      if(strcmp(ip, my_ips[i]) == 0) {
        printf("Can't connect to self\n");
        return -1;
      }
    }
  }

  // check if connection already exists
  cur_clients_for_each(check_for_ip, (void *)ip);

  if (!conn_exists) {

    Client c;

    // initialize SensorData for each Sensor
    for (int j = 0; j < SENSORCOUNT; j++) {
      c.data[j] = (SensorData) { DBL_MIN, DBL_MIN, DBL_MIN};
    }

    int client_sock = socket(AF_INET, SOCK_STREAM, 0);

    SockAddr peer;
    peer.sin_addr.s_addr = inet_addr(ip);
    peer.sin_family      = AF_INET;
    peer.sin_port        = htons(port);

    if (connect(
      client_sock,
      (struct sockaddr *)&peer,
      sizeof(peer)
    ) < 0 ) {
      printf("ERROR: connecting to peer failed\n");
      return -1;
    }

    memcpy(c.ip,inet_ntoa(peer.sin_addr), sizeof(c.ip));
    c.sock = client_sock;

    printf("Connection established\n");

    put(c);
    return 1;
  }

  printf("Connection already established\n");
  conn_exists = 0;
  return 0;
}

// }}}

// A 3.1 DISCOVERY
// {{{

void *discovery_broadcast(void *a) {

  int port = *(int *)a;
  int socket_description;

  socket_description = socket(AF_INET, SOCK_DGRAM, 0);
  if(socket_description == -1) {
    printf("ERROR: could not create discovery broadcast socket\n");
    return NULL;
  }

  // SET TO BROADCAST
  int enable = 1;
  if(
    setsockopt(socket_description, SOL_SOCKET, SO_BROADCAST, &enable, sizeof(int)) < 0
  ){
    printf("ERROR: can't set broadcast option for discovery socket\n");
    return NULL;
  }

  // socket_description is not bound -> OS will do the binding

  // create broadcast socket
  SockAddr udp_broadcast;
  udp_broadcast.sin_family = AF_INET;
  udp_broadcast.sin_addr.s_addr = inet_addr("255.255.255.255");
  udp_broadcast.sin_port = htons(4567);

  char sendmsg[BUFSIZE];
  sprintf(sendmsg,"%ld", port);

  while(1) {
    sleep(10);
    if(
      sendto(
        socket_description,
        sendmsg,
        strlen(sendmsg),
        0,
        (struct sockaddr *)&udp_broadcast,
        sizeof(udp_broadcast)
      ) < 0
    ) {
      printf("ERROR: can't send udp broadcast\n");
    }
  }
}

void *discovery_recv(void *a) {

  int socket_description;

  SockAddr udp_addr, udp_from;
  int sock_size = sizeof(udp_from);

  udp_addr.sin_family = AF_INET;
  udp_addr.sin_addr.s_addr = inet_addr(IPADDR);
  udp_addr.sin_port = htons(4567);

  socket_description = socket(AF_INET, SOCK_DGRAM, 0);
  if(socket_description == -1) {
    printf("ERROR: could not create discovery listener socket\n");
    return NULL;
  }

  if (
    bind(
      socket_description,
      (struct sockaddr *)&udp_addr,
      sizeof(udp_addr)
    ) < 0
  ) {
    printf("ERROR: binding discovery listener failed\n");
    return NULL;
  }

  char recvmsg[BUFSIZE];

  while(1) {
    // listen or something
    if (
      recvfrom(
        socket_description,
        recvmsg,
        sizeof(recvmsg),
        0,
        (struct sockaddr *)&udp_from,
        &sock_size
      ) > 0
    ) {
      printf("discovery: %s:%s\n",inet_ntoa(udp_from.sin_addr), recvmsg);
      connect_peer(inet_ntoa(udp_from.sin_addr), atol(recvmsg));
      memset(recvmsg, 0, strlen(recvmsg));
    }
  }
}

// }}}

// pub function for initializing server.
int init_server(int *socket_description, long port) {

  // initialize socket
  //
  // initializes the server socket which
  // is the communication endpoint.
  {
    SockAddr server_addr;
    // set server address family to ip
    server_addr.sin_family = AF_INET;
    // set server ip address to IPADDR with
    // arpa::inet::inet_addr
    server_addr.sin_addr.s_addr = inet_addr(IPADDR);
    // set server port to port with
    // netinet::in::htons (via arpa::inet)
    server_addr.sin_port = htons(port);

    *socket_description = socket(AF_INET, SOCK_STREAM, 0);

    if(*socket_description == -1) {
      printf("ERROR: can't create socket\n");
      return -1;
    }

    if (
      bind(
        *socket_description,
        (struct sockaddr *) &server_addr,
        sizeof(server_addr)
      ) < 0
    ) {
      printf("ERROR: binding failed\n");
      return -1;
    }
  }

  // initialize grovepi and ports
  init_sensors();

  // initialize semaphores
  sem_init(&sem_put, 0, QUEUESIZE);
  sem_init(&sem_take, 0, 0);

  // initialize cur_clients
  for(int i = 0; i < WORKERCOUNT; i++) {
    cur_clients[i].sock = -2;
  }

  // initialize regex for workers_rcv
  {
    if (
      regcomp(
        &r_sensorresp,
        "^((TEMPERATURE|HUMIDITY|LIGHT|DISTANCE|SOUND|ANGLE|MOISTURE)"
        "*:? *)?(([0-9]+\.?[0-9]*)|VOID)[\t\n\r\f\v]*$",
        REG_EXTENDED
      )
    ) {
      printf("ERROR: compiling regex sensorresp\n");
      return -1;
    }
    if (
      regcomp(&r_recognitionresp,"RECOGNIZED",REG_EXTENDED)
    ) {
      printf("ERROR: compiling regex recoginition_resp\n");
      return -1;
    }
    if (
      regcomp(&r_ip,"[[0-9]{1,3}.]{3}[0-9]{1,3}",REG_EXTENDED)
    ) {
      printf("ERROR: compiling regex ip\n");
      return -1;
    }
  }

  // initialize workers
  {
    pthread_t workers_rcv[WORKERCOUNT];
    pthread_t workers_snd[WORKERCOUNT];

    // initialize worker ids
    for(int i = 0; i < WORKERCOUNT; i++) {
      worker_id[i] = i;
    }

    int errno;

    for(int i = 0; i < WORKERCOUNT; i++) {

      errno = pthread_create(&workers_rcv[i], NULL, work_rcv, (void *)&worker_id[i]);
      if (errno) {
        printf("ERROR: can't create worker receive thread\n");
      }

      errno = pthread_create(&workers_snd[i], NULL, work_snd, (void *)&worker_id[i]);
      if (errno) {
        printf("ERROR: can't create worker send thread\n");
      }
    }
  }

  // initialize recognizors
  {
    pthread_t recognizors[APICOUNT];

    //initialize recognizor ids
    for(int i = 0; i < APICOUNT; i++) {
      recognizor_id[i] = i;
    }

    for(int i = 0; i < APICOUNT; i++) {
      int errno = pthread_create(
        &recognizors[i], NULL, recognizor, (void *)&recognizor_id[i]
      );
      if (errno) {
        printf("ERROR: can't create recognizor thread\n");
      }
    }
  }

  // initialize discovery agents
  //
  // (!) NEEDS TO BE REMOVED WHEN TESTING WITH 2ND SERVER
  //     RUNNING ON THE SAME DEVICE (UDP LISTENERS ARE
  //     BLOCKING EACH OTHER)
  //
  {
    pthread_t a_broadcast, a_recv;

    int errno = pthread_create(
      &a_broadcast, NULL, discovery_broadcast, (void*)&port
    );
    if (errno) {
      printf("ERROR: can't create udp_broadcast thread\n");
    }

    errno = pthread_create(
      &a_recv, NULL, discovery_recv, NULL
    );
    if (errno) {
      printf("ERROR: can't create udp_recv thread\n");
    }
  }

  return 0;
}

// pub function that handles incoming clients
// (listens for incoming sockets and puts them
// in the task_queue so worker-threads can
// handle the response).
void handle_client(int *socket_description) {

  Client c;

  // initialize SensorData for each Sensor
  for (int j = 0; j < SENSORCOUNT; j++) {
    c.data[j] = (SensorData) { DBL_MIN, DBL_MIN, DBL_MIN};
  }

  int client_sock;

  // needed for sys::socket::accept
  int sock_size = sizeof(SockAddr);

  SockAddr client_addr;

  listen(*socket_description, 3);

  client_sock = accept(
    *socket_description,
    (struct sockaddr*) &client_addr,
    (socklen_t*)&sock_size
  );

  if (client_sock < 0) {
    printf("ERROR: accepting client failed\n");
    return ;
  }

  // initialize new Client c and put it to task_queue
  memcpy(c.ip,inet_ntoa(client_addr.sin_addr), sizeof(c.ip));
  c.sock = client_sock;

  printf("New client connected: %s\n", c.ip);

  put(c);
}
