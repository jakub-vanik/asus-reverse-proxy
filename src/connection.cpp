#include "connection.h"

#define Checked(f) if(!(f)) return false

Connection::Connection(Logger &logger, Resolver &resolver, Filter &filter, int port, int fd, sockaddr_in *addr) :
    logger(logger), resolver(resolver), filter(filter)
{
  this->port = port;
  serverFd = fd;
  clientFd = -1;
  peerAddr = addr;
  hostName = NULL;
}

Connection::~Connection()
{
  if (hostName)
  {
    free(hostName);
    hostName = NULL;
  }
}

void Connection::Start()
{
  HandleClient();
}

bool Connection::HandleClient()
{
  Checked(ProcessRequest());
  if (hostName)
  {
    logger.Log("port %d, thread %d: hostname is %s", port, pthread_self(), hostName);
  }
  const char *host = resolver.Resolve(hostName);
  if (host)
  {
    if (filter.Validate(hostName, peerAddr))
    {
      logger.Log("port %d, thread %d: connecting to %s", port, pthread_self(), host);
      Checked(ConnectClient(host));
      logger.Log("port %d, thread %d: client connection established", port, pthread_self());
      Checked(inputStream.ReadSeek(0, false));
      int bytes = inputStream.ReadAvailable();
      Checked(inputStream.ReadToSocket(clientFd, inputStream.ReadAvailable()));
      logger.Log("port %d, thread %d: %d bytes resent", port, pthread_self(), bytes);
      inputStream.Reset();
      ProxyConnection();
      return true;
    }
    else
    {
      logger.Log("port %d, thread %d: connection rejected by filter", port, pthread_self());
    }
  }
  else
  {
    logger.Log("port %d, thread %d: unable to resolve hostname", port, pthread_self());
  }
  return false;
}

bool Connection::ConnectClient(const char *host)
{
  sockaddr_in address;
  memset(&address, 0, sizeof(address));
  address.sin_family = AF_INET;
  address.sin_port = htons(port);
  if (inet_pton(AF_INET, host, &address.sin_addr) <= 0)
  {
    return false;
  }
  clientFd = socket(AF_INET, SOCK_STREAM, 0);
  if (clientFd < 0)
  {
    return false;
  }
  if (connect(clientFd, (sockaddr *) &address, sizeof(address)) < 0)
  {
    close(clientFd);
    return false;
  }
  int keepAlive = 1;
  if (setsockopt(clientFd, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(keepAlive)))
  {
    close(clientFd);
    return false;
  }
  return true;
}

void Connection::ProxyConnection()
{
  pthread_t thread = StartThread();
  logger.Log("port %d, thread %d: output thread %d created", port, pthread_self(), thread);
  PipeInput();
  shutdown(clientFd, SHUT_RD);
  logger.Log("port %d, thread %d: client socket blocked", port, pthread_self());
  pthread_join(thread, NULL);
  logger.Log("port %d, thread %d: output thread %d joined", port, pthread_self(), thread);
  close(clientFd);
  logger.Log("port %d, thread %d: client connection closed", port, pthread_self());
}

pthread_t Connection::StartThread()
{
  pthread_t thread;
  while (pthread_create(&thread, NULL, Thread, this) != 0)
  {
    sleep(1);
  }
  return thread;
}

bool Connection::PipeInput()
{
  while (true)
  {
    Checked(inputStream.WriteFromSocket(serverFd, BUFFER_SIZE, false));
    int bytes = inputStream.ReadAvailable();
    Checked(inputStream.ReadToSocket(clientFd, bytes));
    inputStream.Reset();
    logger.Log("port %d, thread %d: %d bytes uploaded", port, pthread_self(), bytes);
  }
}

void Connection::ForwardOutput()
{
  PipeOutput();
  shutdown(serverFd, SHUT_RD);
  logger.Log("port %d, thread %d: server socket blocked", port, pthread_self());
}

bool Connection::PipeOutput()
{
  while (true)
  {
    Checked(outputStream.WriteFromSocket(clientFd, BUFFER_SIZE, false));
    int bytes = outputStream.ReadAvailable();
    Checked(outputStream.ReadToSocket(serverFd, bytes));
    outputStream.Reset();
    logger.Log("port %d, thread %d: %d bytes downloaded", port, pthread_self(), bytes);
  }
}

void *Connection::Thread(void *param)
{
  Connection *connection = (Connection *) param;
  connection->ForwardOutput();
  return NULL;
}

HttpConnection::HttpConnection(Logger &logger, Resolver &resolver, Filter &filter, int port, int fd, sockaddr_in *addr) :
    Connection(logger, resolver, filter, port, fd, addr)
{
}

bool HttpConnection::ProcessRequest()
{
  if (inputStream.WriteFromSocket(serverFd, BUFFER_SIZE, false))
  {
    char line[1024];
    while (inputStream.ReadLine(line, sizeof(line)))
    {
      if (strlen(line) == 2)
      {
        return true;
      }
      if (StartsWith(line, "Host: "))
      {
        char *pointer = line + 6;
        int length = strlen(pointer) - 2;
        if (hostName)
        {
          free(hostName);
          hostName = NULL;
        }
        hostName = (char *) malloc(length + 1);
        if (hostName)
        {
          memset(hostName, 0, length + 1);
          memcpy(hostName, pointer, length);
        }
        return true;
      }
    }
  }
  return false;
}

bool HttpConnection::StartsWith(const char *str, const char *substr)
{
  return memcmp(str, substr, strlen(substr)) == 0;
}

SslConnection::SslConnection(Logger &logger, Resolver &resolver, Filter &filter, int port, int fd, sockaddr_in *addr) :
    Connection(logger, resolver, filter, port, fd, addr)
{
}

bool SslConnection::ProcessRequest()
{
  Checked(inputStream.WriteFromSocket(serverFd, 5, true));
  int contentType;
  Checked(inputStream.ReadNumber(contentType, FIELD_SIZE_BYTE));
  if (contentType == 22)
  {
    Checked(inputStream.ReadSeek(3, false));
    int length;
    Checked(inputStream.ReadNumber(length, FIELD_SIZE_WORD));
    Checked(inputStream.WriteFromSocket(serverFd, length, true));
    int hanshakeType;
    Checked(inputStream.ReadNumber(hanshakeType, FIELD_SIZE_BYTE));
    if (hanshakeType == 1)
    {
      Checked(inputStream.ReadSeek(43, false));
      Checked(SkipBlock(FIELD_SIZE_BYTE));
      Checked(SkipBlock(FIELD_SIZE_WORD));
      Checked(SkipBlock(FIELD_SIZE_BYTE));
      Checked(ParseExtensions());
      return true;
    }
  }
  return false;
}

bool SslConnection::SkipBlock(FieldSize size)
{
  int blockSize;
  Checked(inputStream.ReadNumber(blockSize, size));
  Checked(inputStream.ReadSeek(blockSize, true));
  return true;
}

bool SslConnection::ParseExtensions()
{
  int blockSize;
  Checked(inputStream.ReadNumber(blockSize, FIELD_SIZE_WORD));
  int processedBytes = 0;
  while (processedBytes < blockSize)
  {
    int recordType;
    Checked(inputStream.ReadNumber(recordType, FIELD_SIZE_WORD));
    int recordSize;
    Checked(inputStream.ReadNumber(recordSize, FIELD_SIZE_WORD));
    long position = inputStream.ReadPosition();
    if (recordType == 0)
    {
      Checked(ParseServerName());
    }
    processedBytes += 4 + recordSize;
    Checked(inputStream.ReadSeek(position + recordSize, false));
  }
  return true;
}

bool SslConnection::ParseServerName()
{
  int blockSize;
  Checked(inputStream.ReadNumber(blockSize, FIELD_SIZE_WORD));
  int processedBytes = 0;
  while (processedBytes < blockSize)
  {
    int recordType;
    Checked(inputStream.ReadNumber(recordType, FIELD_SIZE_BYTE));
    int recordSize;
    Checked(inputStream.ReadNumber(recordSize, FIELD_SIZE_WORD));
    long position = inputStream.ReadPosition();
    if (recordType == 0)
    {
      if (hostName)
      {
        free(hostName);
        hostName = NULL;
      }
      hostName = (char *) malloc(recordSize + 1);
      if (hostName)
      {
        memset(hostName, 0, recordSize + 1);
        Checked(inputStream.ReadData(hostName, recordSize));
      }
    }
    processedBytes += 3 + recordSize;
    Checked(inputStream.ReadSeek(position + recordSize, false));
  }
  return true;
}
