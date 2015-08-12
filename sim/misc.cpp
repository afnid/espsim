#include "espsim.h"

#include "time.h"

static void debug2(int line) {
	//os_printf("%d\n", line);
}

extern "C" {
	void ets_delay_us(uint16 delay) {
		if (delay > 10) {
			static struct timespec req;
			static struct timespec rem;

			bzero(&req, sizeof(req));
			bzero(&rem, sizeof(rem));
			req.tv_nsec = delay * 1000;
			uint16 loops = 0;
			//req.tv_sec = req.tv_nsec / nstos;
			//req.tv_nsec -= req.tv_sec * nstos;

			//uint32 t0 = system_get_time();

			do {
				nanosleep(&req, &rem);
				loops++;
			} while (rem.tv_sec > 0 || rem.tv_nsec > 0);

			//uint32 t1 = system_get_time();

			//printf("loops:%d delay:%d %d %d %d\n", loops, delay, t0, t1, t1 - t0);
		}
	}

	void ets_isr_attach(int id, void (*func)(void *arg), void *arg) {
		debug2(__LINE__);
	}

	void ets_isr_mask(int mask) {
		debug2(__LINE__);
	}

	void ets_isr_unmask(int mask) {
		debug2(__LINE__);
	}

	void ets_timer_arm(volatile ETSTimer *ptimer, uint32 ms, bool repeat_flag, int wtf) {
		debug2(__LINE__);
		esp8266 *esp = getesp();
		ptimer->timer_next = esp->timers;
		esp->timers = ptimer;
		ptimer->timer_period = ms;
	}

	void ets_timer_arm_new(volatile ETSTimer *ptimer, uint32 ms, bool repeat_flag, int wtf) {
		debug2(__LINE__);
		esp8266 *esp = getesp();
		ptimer->timer_next = esp->timers;
		esp->timers = ptimer;
		ptimer->timer_period = ms;
	}

	void ets_timer_disarm(volatile ETSTimer *ptimer) {
		debug2(__LINE__);

		esp8266 *esp = getesp();

		if (esp->timers == ptimer)
			esp->timers = esp->timers->timer_next;

		ETSTimer *t = esp->timers;

		while (t) {
			if (t->timer_next == ptimer)
				t->timer_next = t->timer_next->timer_next;

			t = t->timer_next;
		}
	}

	void ets_timer_setfn(volatile ETSTimer *ptimer, ETSTimerFunc *pfunction, void *parg) {
		ptimer->timer_func = pfunction;
		ptimer->timer_arg = parg;
	}

	void gpio_init() {
		debug2(__LINE__);
	}

	void gpio_output_set(uint32 a, uint32 b, uint32 c, uint32 d) {
		//os_printf("%d:gpio_output_set\n", __LINE__);
	}

	uint32 gpio_input_get(void) {
		//debug2(__LINE__);
		return 0;
	}

	void pwm_set_duty() {
		debug2(__LINE__);
	}

	void pwm_start() {
		debug2(__LINE__);
	}

	void pwm_set_freq_duty(uint16 freq, uint8 *duty) {
		debug2(__LINE__);
	}

	uint32 sntp_get_current_timestamp() {
		debug2(__LINE__);
		return 0;
	}

	char* sntp_get_real_time(long t) {
		debug2(__LINE__);
		return 0;
	}

	sint8 sntp_get_timezone(void) {
		debug2(__LINE__);
		return 0;
	}

	bool sntp_set_timezone(sint8 timezone) {
		debug2(__LINE__);
		return 0;
	}

	void sntp_init(void) {
		debug2(__LINE__);
	}

	void sntp_stop(void) {
		debug2(__LINE__);
	}

	void sntp_setserver(unsigned char idx, ip_addr_t *addr) {
		debug2(__LINE__);
	}

	ip_addr_t sntp_getserver(unsigned char idx) {
		debug2(__LINE__);
		ip_addr_t ip;
		bzero(&ip, sizeof(ip));
		return ip;
	}

	void sntp_setservername(unsigned char idx, char *server) {
		debug2(__LINE__);
	}

	char *sntp_getservername(unsigned char idx) {
		debug2(__LINE__);
		return 0;
	}

	uint32 spi_flash_get_id(void) {
		debug2(__LINE__);
		return 0;
	}

	SpiFlashOpResult spi_flash_erase_sector(uint16 sector) {
		debug2(__LINE__);
		esp8266 *esp = getesp();
		os_bzero(esp->flash[sector], SPI_FLASH_SEC_SIZE);
		//printf("erase %d\n", sector);
		return SPI_FLASH_RESULT_OK;
	}

	SpiFlashOpResult spi_flash_write(uint32 des_addr, uint32 *src_addr, uint32 size) {
		debug2(__LINE__);
		esp8266 *esp = getesp();
		uint32 sector = des_addr / SPI_FLASH_SEC_SIZE;
		uint32 mask = SPI_FLASH_SEC_SIZE - 1;
		os_memcpy((uint8*)(esp->flash[sector]) + (des_addr & mask), src_addr, size);
		//printf("write %d\n", sector);
		return SPI_FLASH_RESULT_OK;
	}

	SpiFlashOpResult spi_flash_read(uint32 src_addr, uint32 *des_addr, uint32 size) {
		debug2(__LINE__);
		esp8266 *esp = getesp();
		uint32 sector = src_addr / SPI_FLASH_SEC_SIZE;
		uint32 mask = SPI_FLASH_SEC_SIZE - 1;
		os_memcpy(des_addr, (uint8*)(esp->flash[sector]) + (src_addr & mask), size);
		//printf("read %d\n", sector);
		return SPI_FLASH_RESULT_OK;
	}

	uint32 phy_get_vdd33() {
		debug2(__LINE__);
		return 0;
	}

	int uart_tx_one_char(char ch) {
		putchar(ch);
		return 1;
	}

	void uart_div_modify(uint16 a1, uint32 a2) {
		debug2(__LINE__);
	}

	int os_install_putc1(const void *p) {
		debug2(__LINE__);
		return 0;
	}

	unsigned char *ets_uncached_addr(int addr) {
		esp8266 *esp = getesp();
		addr &= (sizeof(esp->gpio) - 1);
		addr &= ~3;
		return esp->gpio + addr; // silly
	}

	unsigned char *ets_cached_addr(int addr) {
		esp8266 *esp = getesp();
		addr &= (sizeof(esp->gpio) - 1);
		addr &= ~3;
		return esp->gpio + addr; // silly
	}

	extern void user_init();
}
