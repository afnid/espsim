#include "network.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>

// no esp header files comingled, reduces issues

int initAddr(struct sockaddr *addr, uint32_t ip, int port) {
	struct sockaddr_in inaddr;
	socklen_t len = sizeof(inaddr);
	bzero(&inaddr, len);
	inaddr.sin_family = AF_INET;
	inaddr.sin_port = htons(port);
	inaddr.sin_addr.s_addr = ip;
	//memcpy(&inaddr.sin_addr.s_addr, tcp ? conn->proto.tcp->remote_ip : conn->proto.udp->remote_ip, 4);

	memcpy(addr, &inaddr, len);
	return len;
}

unsigned int getipaddr(struct sockaddr *addr) {
	struct sockaddr_in inaddr;
	memcpy(&inaddr, addr, sizeof(inaddr));
	return inaddr.sin_addr.s_addr;
}

int getport(struct sockaddr *addr) {
	struct sockaddr_in inaddr;
	memcpy(&inaddr, addr, sizeof(inaddr));
	return ntohs(inaddr.sin_port);
}

bool getmacaddr(uint8_t *mac) {
	ifreq ifr;
	ifconf ifc;
	char buf[1024];
	int success = 0;

	bzero(mac, 6);

	int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);

	if (fd < 0) {
		perror("socket");
		return false;
	}

	ifc.ifc_len = sizeof(buf);
	ifc.ifc_buf = buf;

	if (ioctl(fd, SIOCGIFCONF, &ifc) < 0) { /* handle error */
		perror("ioctl");
		close(fd);
		return false;
	}

	struct ifreq* it = ifc.ifc_req;
	const struct ifreq* const end = it + (ifc.ifc_len / sizeof(struct ifreq));

	for (; it != end; ++it) {
		strcpy(ifr.ifr_name, it->ifr_name);

		if (ioctl(fd, SIOCGIFFLAGS, &ifr) == 0) {
			if (!(ifr.ifr_flags & IFF_LOOPBACK)) { // don't count loopback
				if (ioctl(fd, SIOCGIFHWADDR, &ifr) == 0) {
					success = 1;
					break;
				}
			}
		} else { /* handle error */
			perror("ioctl");
			close(fd);
			return false;
		}
	}

	if (success)
	memcpy(mac, ifr.ifr_hwaddr.sa_data, 6);

	return success;
}

bool getipinfo(uint32_t *ipaddr, uint32_t *netmask) {
	*ipaddr = 0;
	*netmask = 0;

	struct ifaddrs *ifaddr;

	if (getifaddrs(&ifaddr) >= 0) {
		for (struct ifaddrs *ifa = ifaddr; ifa != NULL ; ifa = ifa->ifa_next) {
			if (!ifa->ifa_addr)
				continue;
			if (ifa->ifa_addr->sa_family != AF_INET)
				continue;
			if (!strcmp(ifa->ifa_name, "lo"))
				continue;

			if (ifa->ifa_addr) {
				struct sockaddr_in *in = (struct sockaddr_in *) (ifa->ifa_addr);
				*ipaddr = in->sin_addr.s_addr;
				printf("ipaddr  %s: %s\n", ifa->ifa_name, fmtaddr(*ipaddr));
			}

			if (ifa->ifa_netmask) {
				struct sockaddr_in *in = (struct sockaddr_in *) (ifa->ifa_netmask);
				*netmask = in->sin_addr.s_addr;
				printf("ipaddr  %s: %s\n", ifa->ifa_name, fmtaddr(*netmask));
			}
		}

		freeifaddrs(ifaddr);
	}

	return true;
}

uint32_t getgateway() {
	uint32_t gateway = 0;
	return gateway;
}

const char *fmtaddr(uint32_t addr) {
	static char buf1[20];
	static char buf2[20];
	static int rotate = 0;

	char *buf = rotate ? buf1 : buf2;
	rotate = !rotate;

	bzero(buf, sizeof(buf));
	struct in_addr in;
	memcpy(&in, &addr, sizeof(addr));
	strcpy(buf, inet_ntoa(in));
	return buf;
}
