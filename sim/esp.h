#ifndef _esp_h_
#define _esp_h_

#ifdef LINUX
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

//#define __uint32_t_defined

#define volatile // solved compile issues

// need to modify soc_eagle.h
#define ETS_UNCACHED_ADDR(addr) ets_uncached_addr(addr)
#define ETS_CACHED_ADDR(addr) ets_cached_addr(addr)

extern "C" {
extern unsigned char *ets_uncached_addr(int addr);
extern unsigned char *ets_cached_addr(int addr);
}
#endif

extern "C" {
#include "ets_sys.h"
#include "user_interface.h"
#include "mem.h"
#include "osapi.h"
#include "gpio.h"

#ifdef OLDVERISON
// missing esp8266 prototypes
uint16 readvdd33(void);
void ets_delay_us(uint16 us);
int os_printf_plus(const char *fmt, ...);

void ets_bzero(void *s, size_t n);
void ets_memset(void *s, int ch, size_t n);
void ets_memcpy(void *des, const void *src, size_t n);
int ets_memcmp(const void *des, const void *src, size_t n);
int ets_memmove(const void *des, const void *src, size_t n);

int ets_sprintf(char *buf, const char *fmt, ...);
int ets_strncmp(const char *des, const char *src, size_t n);
int ets_strcmp(const char *des, const char *src);
int ets_strlen(const char *des);
void ets_strcpy(const char *des, const char *src);
void ets_strncpy(char *des, const char *src, size_t n);
const char *ets_strstr(const char *s, const char *find);
const char *ets_strchr(const char *s, char find);

void *pvPortZalloc(size_t n);
void *pvPortMalloc(size_t n);
void vPortFree(void *p);

void ets_isr_attach(int id, void (*func)(void *arg), void *arg);
void ets_isr_mask(int mask);
void ets_isr_unmask(int mask);


void uart_div_modify(uint16 a1, uint32 a2);
void ets_timer_arm(volatile ETSTimer *ptimer, uint32 milliseconds, bool repeat_flag, int wtf);
void ets_timer_arm_new(volatile ETSTimer *ptimer, uint32 milliseconds, bool repeat_flag, int wtf);
void ets_timer_disarm(volatile ETSTimer *ptimer);
void ets_timer_setfn(volatile ETSTimer *ptimer, ETSTimerFunc *pfunction, void *parg);

uint32 phy_get_vdd33();

int uart_tx_one_char(char ch);
int os_install_putc1(const void *p);
#endif

void user_init();
void uart_hook(char ch);

#ifndef LINUX
int atoi(const char *s);
char *strchr(const char *s, int c);
char *strcat(char *dest, const char *src);
#else

#undef os_random
#define os_random random

#undef os_sprintf
#undef os_printf
#define os_sprintf sprintf
#define os_printf printf
//#define ets_printf printf

#undef os_strcpy
#undef os_strncpy
#undef os_strcmp
#undef os_strncmp
#undef os_strlen
#undef os_strstr
#define os_strcpy strcpy
#define os_strncpy strncpy
#define os_strncmp strncmp
#define os_strcmp strcmp
#define os_strstr strstr
#define os_strlen strlen

#define ets_strcpy strcpy
#define ets_strncpy strncpy
#define ets_strncmp strncmp
#define ets_strcmp strcmp
#define ets_strstr strstr
#define ets_strlen strlen

#undef os_bzero
#undef os_memcmp
#undef os_memset
#undef os_memcpy
#undef os_memmove
#define os_bzero bzero
#define os_memcmp memcmp
#define os_memset memset
#define os_memcpy memcpy
#define os_memmove memmove

#define ets_bzero bzero
#define ets_memcmp memcmp
#define ets_memset memset
#define ets_memcpy memcpy
#define ets_memmove memmove

#undef os_zalloc
#undef os_malloc
#define os_malloc(x) sim_alloc(__FILE__, __LINE__, x)
#define os_zalloc(x) sim_alloc(__FILE__, __LINE__, x)

#undef os_free
#define os_free(x) sim_free(__FILE__, __LINE__, x)

void sim_free(const char *file, int line, void *buf);
void *sim_alloc(const char *file, int line, size_t n);

#endif

}

#endif
