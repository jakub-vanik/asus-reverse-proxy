#ifndef STREAM_H_
#define STREAM_H_

#include <cstdlib>
#include <cstring>
#include <sys/socket.h>

class Stream
{
public:
  Stream();
  ~Stream();
  void Reset();
  bool WriteFromSocket(int fd, int bytes, bool exactly);
  int ReadPosition();
  int ReadAvailable();
  bool ReadSeek(int pos, bool relative);
  bool ReadNumber(int &value, int bytes);
  bool ReadData(char *buff, int bytes);
  bool ReadLine(char *buff, int bytes);
  bool ReadToSocket(int fd, int bytes);
private:
  char *data;
  int size;
  int writePos;
  int readPos;
  bool Allocate(int expectedBytes);
  bool CanRead(int bytes);
};

#endif
