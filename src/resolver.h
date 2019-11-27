#ifndef RESOLVER_H_
#define RESOLVER_H_

#include <cstdio>
#include <cstdlib>
#include <cstring>

class Resolver
{
public:
  Resolver(const char *fileName);
  ~Resolver();
  const char *Resolve(char *hostName);
private:
  struct Record
  {
    char *hostName;
    char *address;
  };
  Record *records;
  int recordsCount;
  void ProcessLine(char *line, int length);
  char *ExtractString(char *line, int lineLength, int &position, int &stringLength);
  bool AddRecord(char *hostName, int hostNameLength, char *address, int addressLength);
};

#endif
