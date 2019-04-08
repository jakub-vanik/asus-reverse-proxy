#include "server.h"

ServerBase::ServerBase(Logger &logger, Resolver &resolver, int port) :
    logger(logger), resolver(resolver)
{
  this->port = port;
  listenFd = socket(AF_INET, SOCK_STREAM, 0);
  if (listenFd < 0)
  {
    return;
  }
  sockaddr_in address;
  memset(&address, 0, sizeof(address));
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_ANY);
  address.sin_port = htons(PORT_OFFSET + port);
  if (bind(listenFd, (sockaddr *) &address, sizeof(address)) < 0)
  {
    close(listenFd);
    return;
  }
  if (listen(listenFd, 128) < 0)
  {
    close(listenFd);
    return;
  }
  sem_init(&semaphore, 0, 16);
  while (pthread_create(&thread, NULL, MasterThread, this) != 0)
  {
    sleep(1);
  }
}

ServerBase::~ServerBase()
{
  shutdown(listenFd, SHUT_RD);
  pthread_join(thread, NULL);
  sem_destroy(&semaphore);
  close(listenFd);
}

bool ServerBase::StartAndWait()
{
  ThreadInfo *first = CreateThread();
  while (first != NULL)
  {
    first = ReduceThread(first);
  }
  return true;
}

ServerBase::ThreadInfo *ServerBase::CreateThread()
{
  ThreadInfo *next = new ThreadInfo();
  next->server = this;
  next->status = THREAD_STATUS_CAN_REDUCE;
  while (pthread_create(&next->thread, NULL, SlaveThread, next) != 0)
  {
    sleep(1);
  }
  logger.Log("port %d, thread %d: thread %d created", port, pthread_self(), next->thread);
  return next;
}

ServerBase::ThreadInfo *ServerBase::ReduceThread(ThreadInfo *next)
{
  next->status = THREAD_STATUS_MUST_TERMINATE;
  ThreadInfo *result;
  pthread_join(next->thread, (void **) &result);
  logger.Log("port %d, thread %d: thread %d joined", port, pthread_self(), next->thread);
  delete next;
  sem_post(&semaphore);
  return result;
}

ServerBase::ThreadInfo *ServerBase::WaitClient(ThreadInfo *current)
{
  sem_wait(&semaphore);
  sockaddr_in address;
  socklen_t size = sizeof(address);
  int fd = accept(listenFd, (sockaddr *) &address, &size);
  if (fd < 0)
  {
    return NULL;
  }
  int keepAlive = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(keepAlive)))
  {
    return NULL;
  }
  logger.Log("port %d, thread %d: client %s connected", port, pthread_self(), inet_ntoa(address.sin_addr));
  ThreadInfo *next = CreateThread();
  StartConnection(fd);
  close(fd);
  logger.Log("port %d, thread %d: client %s disconnected", port, pthread_self(), inet_ntoa(address.sin_addr));
  while (next != NULL && current->status == THREAD_STATUS_CAN_REDUCE)
  {
    next = ReduceThread(next);
  }
  return next;
}

void *ServerBase::MasterThread(void *param)
{
  ServerBase *server = (ServerBase *) param;
  server->StartAndWait();
  return NULL;
}

void *ServerBase::SlaveThread(void *param)
{
  ThreadInfo *current = (ThreadInfo *) param;
  return current->server->WaitClient(current);
}

template<class T>
Server<T>::Server(Logger &logger, Resolver &resolver, int port) :
    ServerBase(logger, resolver, port)
{
}

template<class T>
void Server<T>::StartConnection(int fd)
{
  T connection(logger, resolver, port, fd);
  connection.Start();
}

HttpServer::HttpServer(Logger &logger, Resolver &resolver, int port) :
    Server<HttpConnection>(logger, resolver, port)
{
}

SslServer::SslServer(Logger &logger, Resolver &resolver, int port) :
    Server<SslConnection>(logger, resolver, port)
{
}
