#include "logger.h"

Logger::Logger(const char* fileName)
{
#if LOG_ENABLED
  file = fopen(fileName, "w+");
  pthread_mutex_init(&mutex, NULL);
#endif
}

Logger::~Logger()
{
#if LOG_ENABLED
  if (file)
  {
    fclose(file);
  }
  pthread_mutex_destroy(&mutex);
#endif
}

void Logger::Log(const char *format, ...)
{
#if LOG_ENABLED
  if (file)
  {
    va_list args;
    va_start(args, format);
    time_t rawtime;
    time(&rawtime);
    tm *timeinfo = localtime(&rawtime);
    char buffer[128];
    memset(buffer, ' ', sizeof(buffer));
    int position = strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S ", timeinfo);
    unsigned int length = vsnprintf(buffer + position, sizeof(buffer) - position, format, args);
    if (length >= 0 && length < sizeof(buffer) - position)
    {
      buffer[length + position] = ' ';
      buffer[sizeof(buffer) - 1] = '\n';
      pthread_mutex_lock(&mutex);
      if (ftell(file) >= LOG_CAPACITY * 128)
      {
        rewind(file);
      }
      int count = fwrite(buffer, sizeof(buffer), 1, file);
      if (count < 1)
      {
        rewind(file);
        clearerr(file);
        fwrite(buffer, sizeof(buffer), 1, file);
      }
      pthread_mutex_unlock(&mutex);
    }
    va_end(args);
  }
#endif
}
