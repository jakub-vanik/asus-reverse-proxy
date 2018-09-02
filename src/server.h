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
#include "logger.h"
#include "resolver.h"

class ServerBase
{
public:
  ServerBase(Logger &logger, Resolver &resolver, int port);
  virtual ~ServerBase();
protected:
  struct ThreadInfo
  {
    pthread_t thread;
    ServerBase* server;
    volatile bool reduce;
  };
  Logger &logger;
  Resolver &resolver;
  int port;
  int listenFd;
  pthread_t thread;
  sem_t semaphore;
  bool StartAndWait();
  ThreadInfo *CreateThread();
  ThreadInfo *ReduceThread(ThreadInfo *next);
  ThreadInfo *WaitClient(ThreadInfo *current);
  virtual void StartConnection(int fd) = 0;
  static void *MasterThread(void *param);
  static void *SlaveThread(void *param);
};

template<class T>
class Server: public ServerBase
{
public:
  Server(Logger &logger, Resolver &resolver, int port);

protected:
  void StartConnection(int fd);
};

class HttpServer: public Server<HttpConnection>
{
public:
  HttpServer(Logger &logger, Resolver &resolver, int port);
};

class SslServer: public Server<SslConnection>
{
public:
  SslServer(Logger &logger, Resolver &resolver, int port);
};

#endif
