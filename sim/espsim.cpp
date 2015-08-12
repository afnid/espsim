#define EXTERN

#include "espsim.h"

#include <signal.h>
#include <getopt.h>
#include <fcntl.h>

uint8 UartDev[1024 * 8];

esp8266 *getesp() {
	static esp8266 *esp = 0;

	if (!esp) {
		esp = ZALLOC(esp8266);
		esp->gpio = (uint8 *) calloc(1024, 8);
		esp->rtc = (uint8 *) calloc(256 + 512, 1);

		esp->blocks = 1024; // 4 mb
		esp->flash = (uint8 **) calloc(sizeof(void *), esp->blocks);

		esp->heap = 20 * 1000;

		// segmented to fail fast on overwrites!

		for (int i = 0; i < esp->blocks; i++)
			esp->flash[i] = (uint8 *) calloc(SPI_FLASH_SEC_SIZE, 1);
	}

	return esp;
}

static bool available(int fd) {
	struct timeval tv;
	fd_set fds;
	FD_ZERO(&fds);
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	FD_SET(fd, &fds);
	select(fd + 1, &fds, NULL, NULL, &tv);
	return (FD_ISSET(0, &fds));
}

static char readch(int fd) {
	if (available(fd)) { // getting false input indicators??
		char ch = 0;
		int n = read(fd, &ch, sizeof(ch));
		return n == 1 ? ch : 0;
	}

	return 0;
}

extern "C" {
	void *sim_alloc(const char *file, int line, size_t n) {
		esp8266 *esp = getesp();

		if (esp->heap > n) {
			uint8 *buf = (uint8 *) calloc(n + 4, 1);
			*(uint32*) buf = n;

			if (trace_allocs)
				printf("%s:%d alloc %d\n", file, line, n);

			esp->heap -= n;

			return buf + 4;
		} else
			printf("%s:%d out of memory %d\n", file, line, n);

		return 0;
	}

	void sim_free(const char *file, int line, void *buf) {
		buf = (uint8 *) buf - 4;
		int n = *(uint32*) buf;

		if (trace_allocs)
			printf("%s:%d free %d\n", file, line, n);

		esp8266 *esp = getesp();
		esp->heap += n;

		free(buf);
	}
}

extern void check_sockets();
extern void stop_sockets();

static void sighandler(int sig) {
	esp8266 *esp = getesp();

	for (int i = 0; i < esp->blocks; i++)
		free(esp->flash[i]);

	free(esp->gpio);
	free(esp->rtc);
	free(esp->flash);
	free(esp);

	stop_sockets();

	printf("graceful shutdown!\n");

	exit(0);
}

static void parseargs(int argc, char **argv) {
	trace_espconn = 0;
	trace_allocs = 0;

	static struct option long_options[] = {
			{ "malloc", no_argument, 0, 'm' },
			{ "espconn", no_argument, 0, 'e' },
			{ "heap", required_argument, 0, 'h' },
			{ 0 }
	};

	int option_index;
	int opt = 0;

	while ((opt = getopt_long(argc, argv, "h:em", long_options, &option_index)) != -1) {
		if (opt == -1)
			break;

		switch (opt) {
			case 0:
				printf("option %s", long_options[option_index].name);
				if (optarg)
					printf(" with arg %s", optarg);
				printf("\n");
				break;
			case 'e':
				trace_espconn = true;
				break;
			case 'm':
				trace_allocs = true;
				break;
			case 'h':
				getesp()->heap = atoi(optarg);
				break;

			default: /* '?' */
				fprintf(stderr, "Usage: %s [-t nsecs] [-n] [addr file]\n", argv[0]);
				fprintf(stderr, "\t-m - trace memory allocation/free calls\n");
				fprintf(stderr, "\t-e - trace networking calls\n");
				fprintf(stderr, "\t-h <bytes> - set the starting heap size\n");
				fprintf(stderr, "\taddr file - like esptool.py, hex address and file to load\n");
				exit(EXIT_FAILURE);
		}
	}

	unsigned int addr = 0;

	for (int i = optind; i < argc; i++) {
		const char *arg = argv[i];

		if (!strncmp(arg, "0x", 2))
			sscanf(arg + 2, "%x", &addr);
		else if (access(arg, R_OK) >= 0) {
			int fd = open(arg, O_RDONLY);

			if (fd > 0) {
				printf("loading file %s at 0x%x\n", arg, addr);
				uint8 buf[SPI_FLASH_SEC_SIZE];
				int total = 0;
				int n = 0;

				while ((n = read(fd, buf, sizeof(buf))) > 0) {
					spi_flash_erase_sector(addr / SPI_FLASH_SEC_SIZE);
					spi_flash_write(addr, (uint32*) buf, n);
					total += n;
					addr += n;
				}

				if (n < 0)
					perror("read");

				close(fd);

				printf("loaded %d bytes, addr %x\n", total, addr);
			}
		}
	}
}

int main(int argc, char **argv) {
	signal(SIGTERM, sighandler);
	signal(SIGINT, sighandler);

	parseargs(argc, argv);

	ip_info ip;
	uint8 mac[6];

	wifi_get_ip_info(STATION_IF, &ip);
	wifi_get_macaddr(STATION_IF, mac);

	printf("ipaddr  %s\n", fmtaddr(ip.ip.addr));
	printf("netmask %s\n", fmtaddr(ip.netmask.addr));
	printf("mac     %x.%x.%x.%x.%x.%x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	user_init();

	while (true) {
		esp8266 *esp = getesp();
		ETSTimer *t = esp->timers;

		if (available(0)) {
			int ch = readch(0);

			if (ch > 0)
				uart_hook(ch);
		}

		while (t) {
			int n = 0;

			if (t->timer_func && system_get_time() > t->timer_expire) {
				//printf("timer %d %d\n", n, t->timer_period);
				t->timer_func(t->timer_arg);
				t->timer_expire = system_get_time() + t->timer_period * 1000;
				n++;
			}

			t = t->timer_next;
		}

		for (int j = 0; j < MAX_TASKS; j++) {
			MyTask *task = esp->tasks + j;

			for (int i = 0; i < task->count; i++) {
				//printf("calling %d\n", i);
				task->task(task->queue + i);
			}

			task->count = 0;
		}

		check_sockets();
	}

	return 0;
}
