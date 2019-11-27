#include "stream.h"

Stream::Stream()
{
  data = NULL;
  size = 0;
  writePos = 0;
  readPos = 0;
}

Stream::~Stream()
{
  if (data)
  {
    free(data);
    data = NULL;
  }
}

void Stream::Reset()
{
  writePos = 0;
  readPos = 0;
}

bool Stream::WriteFromSocket(int fd, int bytes, bool exactly)
{
  if (!Allocate(bytes))
  {
    return false;
  }
  while (bytes > 0)
  {
    char *pointer = data + writePos;
    int count = recv(fd, pointer, bytes, 0);
    if (count <= 0)
    {
      return false;
    }
    if (exactly)
    {
      bytes -= count;
    } else
    {
      bytes = 0;
    }
    writePos += count;
  }
  return true;
}

int Stream::ReadPosition()
{
  return readPos;
}

int Stream::ReadAvailable()
{
  return writePos - readPos;
}

bool Stream::ReadSeek(int pos, bool relative)
{
  if (relative)
  {
    pos += readPos;
  }
  if (pos < 0 || pos > writePos)
  {
    return false;
  }
  readPos = pos;
  return true;
}

bool Stream::ReadNumber(int &value, FieldSize size)
{
  int bytes = static_cast<int>(size);
  if (!CanRead(bytes))
  {
    return false;
  }
  char *pointer = data + readPos;
  value = 0;
  for (int i = 0; i < bytes; i++)
  {
    value <<= 8;
    value += (unsigned char) pointer[i];
  }
  readPos += bytes;
  return true;
}

bool Stream::ReadData(char *buff, int bytes)
{
  if (!CanRead(bytes))
  {
    return false;
  }
  char *pointer = data + readPos;
  memcpy(buff, pointer, bytes);
  readPos += bytes;
  return true;
}

bool Stream::ReadLine(char *buff, int bytes)
{
  char *pointer = data + readPos;
  int aviailableBytes = writePos - readPos;
  for (int i = 0; i < aviailableBytes; i++)
  {
    if (pointer[i] == '\n')
    {
      int count = (i + 1) < (bytes - 1) ? (i + 1) : (bytes - 1);
      memcpy(buff, pointer, count);
      buff[count] = 0;
      readPos += i + 1;
      return true;
    }
  }
  return false;
}

bool Stream::ReadToSocket(int fd, int bytes)
{
  while (bytes > 0)
  {
    char *pointer = data + readPos;
    int count = send(fd, pointer, bytes, MSG_NOSIGNAL);
    if (count <= 0)
    {
      return false;
    }
    bytes -= count;
    readPos += count;
  }
  return true;
}

bool Stream::Allocate(int expectedBytes)
{
  int freeBytes = size - writePos;
  int needAlocate = expectedBytes - freeBytes;
  if (needAlocate > 0)
  {
    int newSize = size + needAlocate;
    if (newSize > 0)
    {
      char *newData = (char *) realloc(data, newSize);
      if (newData == NULL)
      {
        return false;
      }
      data = newData;
      size = newSize;
    }
  }
  return true;
}

bool Stream::CanRead(int bytes)
{
  return readPos + bytes <= writePos;
}
