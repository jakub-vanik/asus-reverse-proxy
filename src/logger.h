#ifndef LOGGER_H_
#define LOGGER_H_

#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <pthread.h>
#include "config.h"

class Logger
{
public:
  Logger(const char* fileName);
  ~Logger();
  void Log(const char *format, ...);
private:
#if LOG_ENABLED
  FILE *file;
  pthread_mutex_t mutex;
#endif
};

#endif
