#include "iptables.h"

void Iptables::ForwardPort(int port)
{
  char buffer[1024];
  snprintf(buffer, sizeof(buffer), "iptables -C INPUT -p tcp -m tcp --dport %d -j ACCEPT", PORT_OFFSET + port);
  if (system(buffer) != 0)
  {
    snprintf(buffer, sizeof(buffer), "iptables -I INPUT -p tcp -m tcp --dport %d -j ACCEPT", PORT_OFFSET + port);
    system(buffer);
  }
  snprintf(buffer, sizeof(buffer), "iptables -t nat -C VSERVER -p tcp -m tcp --dport %d -j REDIRECT --to-ports %d", port, PORT_OFFSET + port);
  if (system(buffer) != 0)
  {
    snprintf(buffer, sizeof(buffer), "iptables -t nat -I VSERVER -p tcp -m tcp --dport %d -j REDIRECT --to-ports %d", port, PORT_OFFSET + port);
    system(buffer);
  }
}

void Iptables::ReleasePort(int port)
{
  char buffer[1024];
  snprintf(buffer, sizeof(buffer), "iptables -D INPUT -p tcp -m tcp --dport %d -j ACCEPT", 5000 + port);
  system(buffer);
  snprintf(buffer, sizeof(buffer), "iptables -t nat -D VSERVER -p tcp -m tcp --dport %d -j REDIRECT --to-ports %d", port, PORT_OFFSET + port);
  system(buffer);
}
