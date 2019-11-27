#ifndef IPTABLES_H_
#define IPTABLES_H_

#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include "config.h"

class Iptables
{
public:
  static void ForwardPort(int port);
  static void ReleasePort(int port);
};

#endif
