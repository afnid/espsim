#ifndef _network_h_
#define _network_h_

bool getmacaddr(unsigned char *mac);
bool getipinfo(unsigned int *ipaddr, unsigned int *netmask);
const char* fmtaddr(unsigned int addr);

struct sockaddr;

int initAddr(struct sockaddr *addr, unsigned int ip, int port);
unsigned int getipaddr(struct sockaddr *addr);
int getport(struct sockaddr *addr);

#endif
