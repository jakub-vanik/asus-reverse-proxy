#ifndef SERVER_H_
#define SERVER_H_

#include <cstdlib>
#include <cstring>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <unistd.h>
#include "config.h"
#include "connection.h"
#include "filter.h"
#include "logger.h"
#include "resolver.h"

class ServerBase
{
public:
  ServerBase(Logger &logger, Resolver &resolver, Filter &filter, int port);
  virtual ~ServerBase();
protected:
  enum ThreadStatus
  {
    THREAD_STATUS_MUST_TERMINATE,
    THREAD_STATUS_CAN_REDUCE
  };
  struct ThreadInfo
  {
    pthread_t thread;
    ServerBase* server;
    volatile ThreadStatus status;
  };
  Logger &logger;
  Resolver &resolver;
  Filter &filter;
  int port;
  int listenFd;
  pthread_t thread;
  sem_t semaphore;
  bool StartAndWait();
  ThreadInfo *CreateThread();
  ThreadInfo *ReduceThread(ThreadInfo *next);
  ThreadInfo *WaitClient(ThreadInfo *current);
  virtual void StartConnection(int fd, sockaddr_in *address) = 0;
  static void *MasterThread(void *param);
  static void *SlaveThread(void *param);
};

template<class T>
class Server: public ServerBase
{
public:
  Server(Logger &logger, Resolver &resolver, Filter &filter, int port);

protected:
  void StartConnection(int fd, sockaddr_in *addr);
};

class HttpServer: public Server<HttpConnection>
{
public:
  HttpServer(Logger &logger, Resolver &resolver, Filter &filter, int port);
};

class SslServer: public Server<SslConnection>
{
public:
  SslServer(Logger &logger, Resolver &resolver, Filter &filter, int port);
};

#endif
