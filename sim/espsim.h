#ifndef _espsim_h_
#define _espsim_h_

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "esp.h"
#include "network.h"

typedef struct _MyTask {
	os_task_t task;
	uint8 prio;os_event_t *queue;
	uint8 qlen;
	uint8 count;
} MyTask;

#define MAX_TASKS 3

typedef struct _esp8266 {
	uint8 *gpio;
	uint8 *rtc;

	uint8 **flash;
	int blocks;

	size_t heap;

	MyTask tasks[MAX_TASKS];
	ETSTimer *timers;

	struct station_config station;
	struct softap_config softap;

	ip_info ipinfo[2];
	uint8 macaddr[6];

	uint8 upgrade; // system_upgrade
} esp8266;

esp8266 *getesp();

#define ZALLOC(x)	((x *)calloc(sizeof(x), 1))

#ifndef EXTERN
#define EXTERN extern
#endif

EXTERN bool trace_espconn;
EXTERN bool trace_allocs;

#endif
