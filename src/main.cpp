#include <cstdlib>
#include <signal.h>
#include <unistd.h>
#include "iptables.h"
#include "logger.h"
#include "server.h"

volatile bool running = true;

void term(int signum)
{
  running = false;
}

int main(void)
{
  struct sigaction action;
  memset(&action, 0, sizeof(struct sigaction));
  action.sa_handler = term;
  sigaction(SIGINT, &action, NULL);
  sigaction(SIGTERM, &action, NULL);
  Logger logger((const char *) "./router.log");
  Resolver resolver((const char *) "./hosts");
  Filter filter((const char *) "./filter.conf");
  HttpServer httpServer(logger, resolver, filter, 80);
  SslServer sslServer(logger, resolver, filter, 443);
  while (running)
  {
    Iptables::ForwardPort(80);
    Iptables::ForwardPort(443);
    sleep(60);
  }
  Iptables::ReleasePort(80);
  Iptables::ReleasePort(443);
  return 0;
}
