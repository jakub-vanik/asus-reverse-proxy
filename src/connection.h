#ifndef CONNECTION_H_
#define CONNECTION_H_

#include <arpa/inet.h>
#include <cstdlib>
#include <netinet/in.h>
#include <sys/socket.h>
#include <pthread.h>
#include <unistd.h>
#include "config.h"
#include "filter.h"
#include "logger.h"
#include "resolver.h"
#include "stream.h"

class Connection
{
public:
  Connection(Logger &logger, Resolver &resolver, Filter &filter, int port, int fd, sockaddr_in *addr);
  virtual ~Connection();
  void Start();
protected:
  Logger &logger;
  Resolver &resolver;
  Filter &filter;
  int port;
  int serverFd;
  int clientFd;
  sockaddr_in *peerAddr;
  char *path;
  char *hostName;
  Stream inputStream;
  Stream outputStream;
  bool HandleClient();
  virtual bool ProcessRequest() = 0;
  bool ConnectClient(const char *host);
  void ProxyConnection();
  pthread_t StartThread();
  bool PipeInput();
  void ForwardOutput();
  bool PipeOutput();
  static void *Thread(void *param);
};

class HttpConnection: public Connection
{
public:
  HttpConnection(Logger &logger, Resolver &resolver, Filter &filter, int port, int fd, sockaddr_in *addr);
protected:
  bool ProcessRequest();
  bool ParseFirstLine(const char *line);
  bool ParseHostLine(const char *line);
  bool StartsWith(const char *str, const char *substr);
};

class SslConnection: public Connection
{
public:
  SslConnection(Logger &logger, Resolver &resolver, Filter &filter, int port, int fd, sockaddr_in *addr);
protected:
  bool ProcessRequest();
  bool ProcessHello();
  bool SkipBlock(FieldSize size);
  bool ParseExtensions();
  bool ParseServerName();
};

#endif
