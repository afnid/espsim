#include "espsim.h"

#include <errno.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>

extern "C" {
#include "espconn.h"
}

#ifndef min
#define min(a, b)	(a < b ? a : b)
#endif

typedef struct _MySocket {
	espconn *conn;

	espconn_sent_callback sent_cb;
	espconn_connect_callback write_finish_fn;
	espconn_connect_callback connect_cb;
	espconn_recv_callback recv_cb;
	espconn_reconnect_callback recon_cb;
	espconn_connect_callback discon_cb;

	int fd;
	bool server;
	bool freeconn;
	bool tcp;

	struct sockaddr addr;
	socklen_t addrlen;

	struct _MySocket *next;
} MySocket;

typedef struct _SentBuffer {
	MySocket *s;
	void *data;
	int len;

	struct _SentBuffer *next;
} SentBuffer;

static MySocket *sockets[1024];
static MySocket *connections = 0;
static SentBuffer *buffers = 0;
static int fdcount = 0;

#define socketdebug(addr, fmt, ...) _debug(__FILE__, __LINE__, addr, fmt,  ## __VA_ARGS__)
#define debug(fmt, ...) _debug(__FILE__, __LINE__, 0, fmt,  ## __VA_ARGS__)
#define socketdebug2(s) socketdebug(&s->addr, "fd:%d proto:%s server:%d\n", s->fd, s->tcp ? "tcp" : "udp", s->server)

static const int PACKET_LEN = 1460;

static void _debug(const char *file, int line, struct sockaddr *addr, const char *fmt, ...) {
	char buf[1024];

	sprintf(buf, "ESPCONN %s:%d - ", file, line);

	if (addr) {
		int port = getport(addr);
		uint32 ip = getipaddr(addr);
		sprintf(buf + strlen(buf), "%s:%d - ", fmtaddr(ip), port);
	}

	int len = strlen(buf);

	va_list va;
	va_start(va, fmt);
	vsnprintf(buf + len, sizeof(buf) - len - 1, fmt, va);

	if (trace_espconn) {
		fwrite(buf, strlen(buf), 1, stdout);
		fflush (stdout);
	}
}

static int queueBuffer(MySocket *s, void *data, int len) {
	SentBuffer *b = ZALLOC(SentBuffer);

	if (b) {
		assert(s->fd > 0);

		b->s = s;
		b->data = data;
		b->len = len;
		b->next = 0;

		if (!buffers) {
			buffers = b;
		} else {
			SentBuffer *tail = buffers;

			while (tail->next)
				tail = tail->next;

			tail->next = b;
		}

		return 0;
	}

	return -1;
}

static SentBuffer *dequeueBuffer() {
	SentBuffer *b = buffers;

	if (buffers)
		buffers = buffers->next;

	return b;
}

static void setSocket(int fd, MySocket *s) {
	s->fd = fd;
	sockets[fd] = s;

	if (fd + 1 > fdcount)
		fdcount = fd + 1;
}

static void clearSocket(MySocket *s) {
	socketdebug2(s);

	if (s->discon_cb)
		s->discon_cb(s->conn);

	if (s == connections) {
		connections = s->next;
	} else {
		MySocket *node = connections;

		while (node->next) {
			if (node->next == s) {
				node->next = node->next->next;
				break;
			}

			node = node->next;
		}
	}

	sockets[s->fd] = 0;

	if (s->conn->state != ESPCONN_CLOSE) {
		s->conn->state = ESPCONN_CLOSE;

		if (s->fd <= 0)
			socketdebug2(s);
		else if (close(s->fd) < 0)
			perror("close");
	}

	if (s->freeconn) {
		bzero(s->conn, sizeof(struct espconn));
		free(s->conn);
	}

	bzero(s, sizeof(MySocket));
	free(s);

	for (int i = 0; i < 1024; i++)
		if (sockets[i])
			fdcount = i + 1;
}

static MySocket *findSocket(espconn *conn, bool create) {
	MySocket *node = connections;

	while (node) {
		if (node->conn == conn)
			return node;
		node = node->next;
	}

	if (create) {
		node = ZALLOC(MySocket);
		node->conn = conn;

		node->next = connections;
		connections = node;
	}

	return node;
}

static MySocket *findSocket(espconn *conn) {
	return findSocket(conn, true);
}

static MySocket *newClient(MySocket *s, int fd, struct sockaddr *addr, socklen_t addrlen) {
	struct espconn *conn = ZALLOC(struct espconn);
	MySocket *sock = findSocket(conn);

	sock->conn = conn;
	sock->freeconn = true;
	memcpy(sock->conn, s->conn, sizeof(struct espconn));
	memcpy(&sock->addr, addr, addrlen);
	sock->addrlen = addrlen;
	sock->fd = fd;
	sock->tcp = s->tcp;
	setSocket(fd, sock);
	return sock;
}

static int initListenAddr(espconn *conn, struct sockaddr *addr) {
	bool tcp = conn->type == ESPCONN_TCP;
	int port = tcp ? conn->proto.tcp->local_port : conn->proto.udp->local_port;

	if (port == 80)
		port += 7000;

	return initAddr(addr, 0, port);
}

static int initClientAddr(espconn *conn, struct sockaddr *addr) {
	uint32 ip;
	bool tcp = conn->type == ESPCONN_TCP;
	int port = tcp ? conn->proto.tcp->remote_port : conn->proto.udp->remote_port;
	memcpy(&ip, tcp ? conn->proto.tcp->remote_ip : conn->proto.udp->remote_ip, 4);
	return initAddr(addr, ip, port);
}

static int listenSocket(struct espconn *conn) {
	MySocket *s = findSocket(conn);
	s->tcp = conn->type == ESPCONN_TCP;
	s->server = true;

	int fd = socket(AF_INET, s->tcp ? SOCK_STREAM : SOCK_DGRAM, 0);

	if (fd < 0) {
		perror("socket");
		return -1;
	}

	s->addrlen = initListenAddr(s->conn, &s->addr);

	socketdebug(&s->addr, "listen %s", s->tcp ? "tcp" : "udp");

	if (bind(fd, &s->addr, s->addrlen) < 0) {
		perror("bind");
		close(fd);
		return -1;
	}

	if (s->tcp && listen(fd, 10) < 0) {
		perror("listen");
		close(fd);
		return -1;
	}

	setSocket(fd, s);

	conn->state = ESPCONN_LISTEN;

	return 0;
}

static int connectSocket(struct espconn *conn) {
	MySocket *s = findSocket(conn);
	s->tcp = conn->type == ESPCONN_TCP;

	int fd = socket(AF_INET, s->tcp ? SOCK_STREAM : SOCK_DGRAM, 0);

	if (fd < 0) {
		perror("socket");
		return -1;
	}

	s->addrlen = initClientAddr(s->conn, &s->addr);

	socketdebug(&s->addr, "connect fd:%d %s", fd, s->tcp ? "tcp" : "udp");

	if (connect(fd, &s->addr, s->addrlen) < 0) {
		perror("connect");
		return -1;
	}

	setSocket(fd, s);
	conn->state = ESPCONN_CONNECT;

	if (s->connect_cb)
		s->connect_cb(conn);

	return 0;
}

void stop_sockets() {
	SentBuffer *b = 0;

	while ((b = dequeueBuffer()))
		free(b);

	while (connections)
		clearSocket(connections);

	bzero(sockets, sizeof(sockets));
}

void check_sockets() {
	struct timeval tv;
	bzero(&tv, sizeof(tv));

	SentBuffer *b = 0;

	while ((b = dequeueBuffer())) {
		if (b->s->tcp) {
			if (write(b->s->fd, b->data, b->len) < 0) {
				socketdebug(&b->s->addr, "write fd=%d len=%d\n", b->s->fd, b->len);
				perror("output");
			} else if (b->s->sent_cb)
				b->s->sent_cb(b->s->conn);
		} else {
			struct sockaddr addr;
			socklen_t addrlen = initClientAddr(b->s->conn, &addr);
			ip_info ii;

			uint32 ip = getipaddr(&addr);
			wifi_get_ip_info(STATION_IF, &ii);

			if ((ip | ii.netmask.addr) == 0xffffffff) // broadcast
				if (setsockopt(b->s->fd, SOL_SOCKET, SO_BROADCAST, &addr, addrlen) < 0) {
					perror("setsockopt (SO_BROADCAST)");
					socketdebug2(b->s);
				}

			if (sendto(b->s->fd, b->data, b->len, 0, &addr, addrlen) < 0) {
				socketdebug(&addr, "sendto fd=%d len=%d\n", b->s->fd, b->len);
				perror("sendto");
			}
		}
		free(b);
	}

	fd_set fds;
	FD_ZERO(&fds);

	for (int i = 0; i < fdcount; i++)
		if (sockets[i])
			FD_SET(i, &fds);

	if (select(fdcount, &fds, NULL, NULL, &tv)) {
		for (int fd = 0; fd < fdcount; fd++) {
			if (FD_ISSET(fd, &fds)) {

				MySocket *s = sockets[fd];

				if (s->tcp) {
					if (s->server) {
						struct sockaddr addr;
						socklen_t addrlen = sizeof(addr);

						int client = accept(s->fd, &addr, &addrlen);

						if (client < 0)
							perror("accept");
						else if (client > 0) {
							MySocket *sock = newClient(s, client, &addr, addrlen);

							if (s->connect_cb)
								s->connect_cb(sock->conn);
						}

						socketdebug(&s->addr, "accept fd=%d client=%d\n", fd, client);
					} else {
						char buf[8192];
						int count = 0;
						int total = 0;
						int len = 0;

						if (ioctl(fd, FIONREAD, &count) < 0)
							perror("ioctl");

						bzero(buf, sizeof(buf));

						while (total < count && (len = read(fd, buf, min(count, PACKET_LEN))) > 0) {
							if (s->recv_cb)
								s->recv_cb(s->conn, buf, len);

							total += len;
						}

						socketdebug(&s->addr, "read fd:%d len:%d\n", fd, total);

						if (total <= 0)
							clearSocket(s);
					}
				} else {
					char buf[8192];
					bzero(buf, sizeof(buf));

					struct sockaddr addr;
					socklen_t addrlen = sizeof(addr);
					int len = recvfrom(fd, buf, sizeof(buf), 0, &addr, &addrlen);

					if (len > 0) {
						uint32 ip = getipaddr(&addr);
						ip_info ii;

						wifi_get_ip_info(STATION_IF, &ii);

						if (ip != ii.ip.addr) {
							//printf("dgram fd:%d %x %x %s %s\n", fd, ip, ii.ip.addr, fmtaddr(ip), fmtaddr(ii.ip.addr));
							memcpy(&s->conn->proto.udp->remote_ip, &ip, sizeof(ip));
							s->conn->proto.udp->remote_port = getport(&addr);
							socketdebug(&s->addr, "dgram fd:%d %s\n", fd, buf);

							if (s->recv_cb)
								s->recv_cb(s->conn, buf, len);
						}
					} else
						perror("recvlen");
				}
			}
		}
	}
}

static void debug1(int line) {
	printf("ESPCONN %s:%d\n", __FILE__, line);
}

extern "C" {
	sint8 espconn_sent(struct espconn *conn, uint8 *psent, uint16 length) {
		MySocket *s = findSocket(conn);
		socketdebug(&s->addr, "sent fd:%d len:%d\n", s->fd, length);
		return queueBuffer(s, psent, length);
	}

	sint8 espconn_send(struct espconn *conn, uint8 *psent, uint16 length) {
		return espconn_sent(conn, psent, length);
	}

	sint8 espconn_secure_send(struct espconn *conn, uint8 *psent, uint16 length) {
		return espconn_sent(conn, psent, length);
	}

	sint8 espconn_connect(struct espconn *conn) {
		MySocket *s = findSocket(conn);
		socketdebug2(s);
		return connectSocket(conn);
	}

	sint8 espconn_disconnect(struct espconn *conn) {
		MySocket *s = findSocket(conn, false);

		if (s) {
			socketdebug2(s);
			clearSocket(s);
		}

		return 0;
	}

	sint8 espconn_delete(struct espconn *conn) {
		MySocket *s = findSocket(conn, false);

		if (s)
			socketdebug2(s);

		return 0;
	}

	sint8 espconn_create(struct espconn *conn) {
		MySocket *s = findSocket(conn);
		socketdebug2(s);
		return listenSocket(conn);
	}

	sint8 espconn_accept(struct espconn *conn) {
		MySocket *s = findSocket(conn);
		socketdebug2(s);
		return listenSocket(conn);
	}

	sint8 espconn_regist_sentcb(struct espconn *conn, espconn_sent_callback sent_cb) {
		MySocket *s = findSocket(conn);
		socketdebug2(s);
		s->sent_cb = sent_cb;
		return 0;
	}

	sint8 espconn_regist_write_finish(struct espconn *conn, espconn_connect_callback write_finish_fn) {
		MySocket *s = findSocket(conn);
		socketdebug2(s);
		s->write_finish_fn = write_finish_fn;
		return 0;
	}

	sint8 espconn_regist_connectcb(struct espconn *conn, espconn_connect_callback connect_cb) {
		MySocket *s = findSocket(conn);
		socketdebug2(s);
		s->connect_cb = connect_cb;
		return 0;
	}

	sint8 espconn_regist_recvcb(struct espconn *conn, espconn_recv_callback recv_cb) {
		MySocket *s = findSocket(conn);
		socketdebug2(s);
		s->recv_cb = recv_cb;
		return 0;
	}

	sint8 espconn_regist_reconcb(struct espconn *conn, espconn_reconnect_callback recon_cb) {
		MySocket *s = findSocket(conn);
		socketdebug2(s);
		s->recon_cb = recon_cb;
		return 0;
	}

	sint8 espconn_regist_disconcb(struct espconn *conn, espconn_connect_callback discon_cb) {
		MySocket *s = findSocket(conn);
		socketdebug2(s);
		s->discon_cb = discon_cb;
		return 0;
	}

	sint8 espconn_regist_time(struct espconn *conn, uint32 interval, uint8 type_flag) {
		MySocket *s = findSocket(conn);
		socketdebug2(s);
		return 0;
	}

	sint8 espconn_secure_accept(struct espconn *conn) {
		debug1(__LINE__);
		return espconn_accept(conn);
	}

	sint8 espconn_secure_connect(struct espconn *conn) {
		MySocket *s = findSocket(conn);
		socketdebug2(s);
		return espconn_connect(conn);
	}

	sint8 espconn_secure_disconnect(struct espconn *conn) {
		MySocket *s = findSocket(conn);
		socketdebug2(s);
		return espconn_disconnect(conn);
	}

	sint8 espconn_secure_sent(struct espconn *conn, uint8 *psent, uint16 length) {
		MySocket *s = findSocket(conn);
		socketdebug2(s);
		return espconn_sent(conn, psent, length);
	}

	uint8 espconn_tcp_get_max_con(void) {
		return 0;
	}

	sint8 espconn_tcp_set_max_con(uint8 num) {
		return 0;
	}

	sint8 espconn_tcp_get_max_con_allow(struct espconn *conn) {
		debug1(__LINE__);
		return 0;
	}

	sint8 espconn_tcp_set_max_con_allow(struct espconn *conn, uint8 num) {
		debug1(__LINE__);
		return 0;
	}

	sint8 espconn_get_connection_info(struct espconn *espconn, remot_info **pcon_info, uint8 typeflags) {
		debug1(__LINE__);
		return 0;
	}

	sint8 espconn_get_packet_info(struct espconn *conn, struct espconn_packet* infoarg) {
		debug1(__LINE__);
		return 0;
	}

	uint32 espconn_port(void) {
		//debug1(__LINE__);
		return 0;
	}

	sint8 espconn_set_opt(struct espconn *conn, uint8 opt) {
		debug1(__LINE__);
		return 0;
	}

	sint8 espconn_clear_opt(struct espconn *conn, uint8 opt) {
		debug1(__LINE__);
		return 0;
	}

	sint8 espconn_set_keepalive(struct espconn *conn, uint8 level, void* optarg) {
		debug1(__LINE__);
		return 0;
	}

	sint8 espconn_get_keepalive(struct espconn *conn, uint8 level, void *optarg) {
		debug1(__LINE__);
		return 0;
	}

	err_t espconn_gethostbyname(struct espconn *espconn, const char *hostname, ip_addr_t *addr, dns_found_callback found) {
		debug1(__LINE__);
		return 0;
	}

	bool espconn_secure_set_size(uint8 level, uint16 size) {
		debug1(__LINE__);
		return 0;
	}

	sint16 espconn_secure_get_size(uint8 level) {
		debug1(__LINE__);
		return 0;
	}

	bool espconn_secure_ca_enable(uint8 level, uint32 flash_sector) {
		debug1(__LINE__);
		return 0;
	}

	bool espconn_secure_ca_disable(uint8 level) {
		debug1(__LINE__);
		return 0;
	}

	sint8 espconn_igmp_join(ip_addr_t *host_ip, ip_addr_t *multicast_ip) {
		debug1(__LINE__);
		return 0;
	}

	sint8 espconn_igmp_leave(ip_addr_t *host_ip, ip_addr_t *multicast_ip) {
		debug1(__LINE__);
		return 0;
	}

	sint8 espconn_recv_hold(struct espconn *conn) {
		MySocket *s = findSocket(conn);
		socketdebug2(s);
		return 0;
	}

	sint8 espconn_recv_unhold(struct espconn *conn) {
		MySocket *s = findSocket(conn);
		socketdebug2(s);
		return 0;
	}

	void espconn_mdns_init(struct mdns_info *info) {
		debug1(__LINE__);
	}

	void espconn_mdns_close(void) {
		debug1(__LINE__);
	}

	void espconn_mdns_server_register(void) {
		debug1(__LINE__);
	}

	void espconn_mdns_server_unregister(void) {
		debug1(__LINE__);
	}

	char* espconn_mdns_get_servername(void) {
		debug1(__LINE__);
		return 0;
	}

	void espconn_mdns_set_servername(const char *name) {
		debug1(__LINE__);
	}

	void espconn_mdns_set_hostname(char *name) {
		debug1(__LINE__);
	}

	char* espconn_mdns_get_hostname(void) {
		debug1(__LINE__);
		return 0;
	}

	void espconn_mdns_disable(void) {
		debug1(__LINE__);
	}

	void espconn_mdns_enable(void) {
		debug1(__LINE__);
	}

	void espconn_dns_setserver(uint8 numdns, ip_addr_t *dnsserver) {
		debug1(__LINE__);
	}
}
