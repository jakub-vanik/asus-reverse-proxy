#ifndef FILTER_H_
#define FILTER_H_

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <netinet/in.h>
#include "config.h"

class Filter
{
public:
  Filter(const char *fileName);
  ~Filter();
  bool Validate(const char *path, const char *hostName, sockaddr_in *peerAddr);
private:
#if FILTER_ENABLED
  enum struct Keyword
  {
    zone,
    host,
    allow,
    deny
  };
  struct RuleRecord
  {
    bool allow;
    bool deny;
    unsigned long address;
    unsigned long mask;
  };
  struct ZoneRecord
  {
    char **hostNames;
    int hostNamesCount;
    RuleRecord *ruleRecords;
    int ruleRecordsCount;
  };
  ZoneRecord *zoneRecords;
  int zoneRecordsCount;
  char *ExtractString(char *line, int lineLength, int &position, int &stringLength);
  bool ParseKeyword(char *text, int length, Keyword &keyword);
  bool ParseAddressRange(char *text, int length, RuleRecord *record);
  bool AddZoneRecord(ZoneRecord **zoneRecord);
  bool AddHostName(ZoneRecord *zoneRecord, char *hostName, int hostNameLength);
  bool AddRuleRecord(ZoneRecord *zoneRecord, RuleRecord *ruleRecord);
  bool MatchRuleRecord(RuleRecord *ruleRecord, sockaddr_in *peerAddr);
#endif
};

#endif
