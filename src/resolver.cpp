#include "resolver.h"

Resolver::Resolver(char *fileName)
{
  records = NULL;
  recordsCount = 0;
  FILE *file = fopen(fileName, "r");
  if (file)
  {
    while (true)
    {
      int position = ftell(file);
      char line[1024];
      int count = fread(line, 1, sizeof(line), file);
      if (count <= 0)
      {
        break;
      }
      for (int i = 0; i < count; i++)
      {
        if (line[i] == '\n')
        {
          ProcessLine(line, i);
          position += i + 1;
          break;
        }
      }
      fseek(file, position, SEEK_SET);
      clearerr(file);
    }
    fclose(file);
  }
}

Resolver::~Resolver()
{
  if (records)
  {
    for (int i = 0; i < recordsCount; i++)
    {
      free(records[i].hostName);
      records[i].hostName = NULL;
      free(records[i].address);
      records[i].address = NULL;
    }
    free(records);
    records = NULL;
  }
}

const char *Resolver::Resolve(char *hostName)
{
  if (hostName)
  {
    for (int i = 0; i < recordsCount; i++)
    {
      if (strcmp(hostName, records[i].hostName) == 0)
      {
        return records[i].address;
      }
    }
  }
  return NULL;
}

void Resolver::ProcessLine(char* line, int length)
{
  int position = 0;
  int addressLength;
  char *address = ExtractString(line, length, position, addressLength);
  while (position < length && line[position] != '#')
  {
    int hostNameLength;
    char *hostName = ExtractString(line, length, position, hostNameLength);
    if (hostNameLength > 0)
    {
      AddRecord(hostName, hostNameLength, address, addressLength);
    }
  }
}

char *Resolver::ExtractString(char* line, int lineLength, int &position, int &stringLength)
{
  stringLength = 0;
  while (position < lineLength && line[position] != '#' && (line[position] == ' ' || line[position] == '\t'))
  {
    position++;
  }
  char *string = line + position;
  while (position < lineLength && line[position] != '#' && (line[position] != ' ' && line[position] != '\t'))
  {
    position++;
    stringLength++;
  }
  return string;
}

bool Resolver::AddRecord(char *hostName, int hostNameLength, char *address, int addressLength)
{
  Record *newRecords = (Record *) realloc(records, (recordsCount + 1) * sizeof(Record));
  if (!newRecords)
  {
    return false;
  }
  records = newRecords;
  records[recordsCount].hostName = (char *) malloc(hostNameLength + 1);
  if (!records[recordsCount].hostName)
  {
    return false;
  }
  memcpy(records[recordsCount].hostName, hostName, hostNameLength);
  records[recordsCount].hostName[hostNameLength] = 0;
  records[recordsCount].address = (char *) malloc(addressLength + 1);
  if (!records[recordsCount].address)
  {
    free(records[recordsCount].hostName);
    records[recordsCount].hostName = NULL;
    return false;
  }
  memcpy(records[recordsCount].address, address, addressLength);
  records[recordsCount].address[addressLength] = 0;
  recordsCount++;
  return true;
}
