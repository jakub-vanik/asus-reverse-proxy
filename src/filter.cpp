#include "filter.h"

#define Checked(f) if(!(f)) exit(EXIT_FAILURE)

Filter::Filter(const char *fileName)
{
#if FILTER_ENABLED
  zoneRecords = NULL;
  zoneRecordsCount = 0;
  FILE *file = fopen(fileName, "r");
  if (file)
  {
    ZoneRecord *zoneRecord = NULL;
    while (true)
    {
      int position = ftell(file);
      char buffer[1024];
      int count = fread(buffer, 1, sizeof(buffer), file);
      if (count <= 0)
      {
        break;
      }
      int bufferPosition = 0;
      int wordLength;
      char *word = ExtractString(buffer, count, bufferPosition, wordLength);
      if (wordLength)
      {
        Keyword keyword;
        Checked(ParseKeyword(word, wordLength, keyword));
        if (keyword == Keyword::zone)
        {
          Checked(AddZoneRecord(&zoneRecord));
        }
        if (keyword == Keyword::host)
        {
          int hostNameLength;
          char *hostName = ExtractString(buffer, count, bufferPosition, hostNameLength);
          Checked(AddHostName(zoneRecord, hostName, hostNameLength));
        }
        if (keyword == Keyword::allow || keyword == Keyword::deny)
        {
          RuleRecord ruleRecord = {0};
          ruleRecord.allow = (keyword == Keyword::allow);
          ruleRecord.deny = (keyword == Keyword::deny);
          int addressRangeLength;
          char *addressRange = ExtractString(buffer, count, bufferPosition, addressRangeLength);
          Checked(ParseAddressRange(addressRange, addressRangeLength, &ruleRecord));
          Checked(AddRuleRecord(zoneRecord, &ruleRecord));
        }
      }
      position += bufferPosition;
      fseek(file, position, SEEK_SET);
      clearerr(file);
    }
    fclose(file);
  }
#endif
}

Filter::~Filter()
{
#if FILTER_ENABLED
  if (zoneRecords)
  {
    for (int i = 0; i < zoneRecordsCount; i++)
    {
      for (int j = 0; j < zoneRecords[i].hostNamesCount; j++)
      {
        free(zoneRecords[i].hostNames[j]);
        zoneRecords[i].hostNames[j] = NULL;
      }
      free(zoneRecords[i].hostNames);
      zoneRecords[i].hostNames = NULL;
      free(zoneRecords[i].ruleRecords);
      zoneRecords[i].ruleRecords = NULL;
    }
    free(zoneRecords);
    zoneRecords = NULL;
  }
#endif
}

bool Filter::Validate(const char *path, const char *hostName, sockaddr_in *peerAddr)
{
#if FILTER_ENABLED
#if FILTER_CERTBOT
  if (path)
  {
    const char *allowed = "/.well-known/acme-challenge/";
    size_t length = strlen(allowed);
    if (strlen(path) >= length && memcmp(path, allowed, length) == 0)
    {
      return true;
    }
  }
#endif
  if (hostName)
  {
    for (int i = 0; i < zoneRecordsCount; i++)
    {
      for (int j = 0; j < zoneRecords[i].hostNamesCount; j++)
      {
        if (strcmp(hostName, zoneRecords[i].hostNames[j]) == 0)
        {
          ZoneRecord *zoneRecord = &zoneRecords[i];
          if (zoneRecord->ruleRecords)
          {
            for (int j = 0; j < zoneRecord->ruleRecordsCount; j++)
            {
              if (MatchRuleRecord(&zoneRecord->ruleRecords[j], peerAddr))
              {
                RuleRecord *ruleRecord = &zoneRecord->ruleRecords[j];
                if (ruleRecord->allow)
                {
                  return true;
                }
                if (ruleRecord->deny)
                {
                  return false;
                }
              }
            }
          }
        }
      }
    }
  }
  return false;
#else
  return true;
#endif
}

#if FILTER_ENABLED

char *Filter::ExtractString(char *buffer, int count, int &position, int &stringLength)
{
  stringLength = 0;
  while (position < count && (buffer[position] == ' ' || buffer[position] == '\t' || buffer[position] == '\n'))
  {
    position++;
  }
  char *string = buffer + position;
  while (position < count && (buffer[position] != ' ' && buffer[position] != '\t' && buffer[position] != '\n'))
  {
    position++;
    stringLength++;
  }
  return string;
}

bool Filter::ParseKeyword(char *text, int length, Filter::Keyword &keyword)
{
  const char *keywords[] = {"zone", "host", "allow", "deny"};
  for (int i = 0; i < 4; i++)
  {
    if (length == (int) strlen(keywords[i]) && memcmp(text, keywords[i], length) == 0)
    {
      keyword = (Keyword) i;
      return true;
    }
  }
  return false;
}

bool Filter::ParseAddressRange(char *text, int length, Filter::RuleRecord *record)
{
  char *start = text;
  char *end = NULL;
  long int numbers[5] = {0};
  for (int i = 0; i < 5; i++)
  {
    numbers[i] = strtol(start, &end, 10);
    if (end == start)
    {
      return false;
    }
    if (end - text > length)
    {
      return false;
    }
    if ((i < 3 && *end != '.') || (i == 3 && *end != '/'))
    {
      return false;
    }
    if (i < 4 && (numbers[i] < 0 || numbers[i] > 255))
    {
      return false;
    }
    if (i == 4 && (numbers[i] < 0 || numbers[i] > 32))
    {
      return false;
    }
    start = end + 1;
  }
  if (end - text != length)
  {
    return false;
  }
  record->address = (numbers[0] << 24) + (numbers[1] << 16) + (numbers[2] << 8) + numbers[3];
  record->mask = (0xFFFFFFFFUL << (32 - numbers[4])) & 0xFFFFFFFFUL;
  return true;
}

bool Filter::AddZoneRecord(Filter::ZoneRecord **zoneRecord)
{
  ZoneRecord *newZoneRecords = (ZoneRecord *) realloc(zoneRecords, (zoneRecordsCount + 1) * sizeof(ZoneRecord));
  if (!newZoneRecords)
  {
    return false;
  }
  zoneRecords = newZoneRecords;
  zoneRecords[zoneRecordsCount].hostNames = NULL;
  zoneRecords[zoneRecordsCount].hostNamesCount = 0;
  zoneRecords[zoneRecordsCount].ruleRecords = NULL;
  zoneRecords[zoneRecordsCount].ruleRecordsCount = 0;
  *zoneRecord = &zoneRecords[zoneRecordsCount];
  zoneRecordsCount++;
  return true;
}

bool Filter::AddHostName(Filter::ZoneRecord *zoneRecord, char *hostName, int hostNameLength)
{
  if (!zoneRecord)
  {
    return false;
  }
  if (hostNameLength == 0)
  {
    return false;
  }
  char **newHostNames = (char **) realloc(zoneRecord->hostNames, (zoneRecord->hostNamesCount + 1) * sizeof(char *));
  if (!newHostNames)
  {
    return false;
  }
  zoneRecord->hostNames = newHostNames;
  char *newHostName = (char *) malloc(hostNameLength + 1);
  if (!newHostName)
  {
    return false;
  }
  memcpy(newHostName, hostName, hostNameLength);
  newHostName[hostNameLength] = 0;
  zoneRecord->hostNames[zoneRecord->hostNamesCount] = newHostName;
  zoneRecord->hostNamesCount++;
  return true;
}

bool Filter::AddRuleRecord(Filter::ZoneRecord *zoneRecord, Filter::RuleRecord *ruleRecord)
{
  if (!zoneRecord)
  {
    return false;
  }
  RuleRecord *newRuleRecords = (RuleRecord *) realloc(zoneRecord->ruleRecords, (zoneRecord->ruleRecordsCount + 1) * sizeof(RuleRecord));
  if (!newRuleRecords)
  {
    return false;
  }
  zoneRecord->ruleRecords = newRuleRecords;
  memcpy(&zoneRecord->ruleRecords[zoneRecord->ruleRecordsCount], ruleRecord, sizeof(RuleRecord));
  zoneRecord->ruleRecordsCount++;
  return true;
}

bool Filter::MatchRuleRecord(Filter::RuleRecord *ruleRecord, sockaddr_in *peerAddr)
{
  unsigned long address = __bswap_32(peerAddr->sin_addr.s_addr);
  if ((address & ruleRecord->mask) == (ruleRecord->address & ruleRecord->mask))
  {
    return true;
  }
  return false;
}

#endif
