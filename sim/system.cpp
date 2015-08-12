#include "espsim.h"

#include <sys/time.h>

static void debug2(int line) {
	//os_printf("%d\n", line);
}

extern "C" {
	uint32 system_get_time() {
		static time_t epoch = 0;
		static struct timeval tv;
		gettimeofday(&tv, 0);

		if (!epoch)
			epoch = tv.tv_sec;

		return (tv.tv_sec - epoch) * 1000000 + tv.tv_usec;
	}

	uint16 system_adc_read(void) {
		debug2(__LINE__);
		return 0;
	}

	uint8 system_get_boot_version(void) {
		debug2(__LINE__);
		return 0;
	}

	uint32 system_get_userbin_addr(void) {
		debug2(__LINE__);
		return 0;
	}

	uint8 system_get_boot_mode(void) {
		debug2(__LINE__);
		return 0;
	}

	uint32 system_get_chip_id() {
		debug2(__LINE__);
		return 0;
	}
	uint16 system_get_vdd33() {
		debug2(__LINE__);
		return 0;
	}
	enum flash_size_map system_get_flash_size_map(void) {
		debug2(__LINE__);
		return FLASH_SIZE_8M_MAP_512_512;
	}
	struct rst_info* system_get_rst_info(void) {
		debug2(__LINE__);
		static rst_info info;
		return &info;
	}

	void system_print_meminfo() {
		debug2(__LINE__);
	}
	uint8 system_get_cpu_freq() {
		debug2(__LINE__);
		return 0;
	}
	const char * system_get_sdk_version() {
		debug2(__LINE__);
		return "ESPSIM-0.1a";
	}
	void system_restore(void) {
		debug2(__LINE__);
	}

	void system_restart(void) {
		debug2(__LINE__);
	}

	bool system_deep_sleep_set_option(uint8 option) {
		debug2(__LINE__);
		return true;
	}

	void system_deep_sleep(uint32 time_in_us) {
		debug2(__LINE__);
	}

	uint32 system_rtc_clock_cali_proc(void) {
		debug2(__LINE__);
		return 0;
	}

	uint32 system_get_rtc_time(void) {
		debug2(__LINE__);
		return 0;
	}

	bool system_rtc_mem_read(uint8 block, void *addr, uint16 len) {
		esp8266 *esp = getesp();
		os_memcpy(addr, esp->rtc + block * 4, len);
		return block * 4u + len < sizeof(esp->rtc);
	}

	bool system_rtc_mem_write(uint8 block, const void *addr, uint16 len) {
		esp8266 *esp = getesp();
		os_memcpy(esp->rtc + block * 4, addr, len);
		return block >= 64u && block * 4u + len < sizeof(esp->rtc);
	}

	void system_timer_reinit(void) {
		debug2(__LINE__);
	}

	uint32 system_get_free_heap_size() {
		esp8266 *esp = getesp();
		return esp->heap;
	}

	bool system_os_task(os_task_t task, uint8 prio, os_event_t *queue, uint8 qlen) {
		os_printf("%d:os_task %d\n", __LINE__, qlen);

		if (prio < MAX_TASKS) {
			esp8266 *esp = getesp();
			MyTask *t = esp->tasks + prio;
			os_bzero(t, sizeof(MyTask));
			t->task = task;
			t->prio = prio;
			t->queue = queue;
			t->qlen = qlen;
		}
		return 0;
	}

	bool system_os_post(uint8 prio, os_signal_t sig, os_param_t par) {
		if (prio < MAX_TASKS) {
			esp8266 *esp = getesp();
			MyTask *t = esp->tasks + prio;

			//os_printf("post %d %d %d\n", prio, t->count, t->qlen);

			if (!t->qlen) {
				exit(0);
			}

			if (t->count < t->qlen) {
				os_event_t *e = t->queue + t->count++;
				e->par = par;
				e->sig = sig;
			}

		}

		return 0;
	}

	void system_upgrade_flag_set(uint8 flag) {
		debug2(__LINE__);
		esp8266 *esp = getesp();
		esp->upgrade = flag;
	}

	uint8 system_upgrade_userbin_check(void) {
		debug2(__LINE__);
		return 0;
	}

	void system_upgrade_reboot(void) {
		debug2(__LINE__);
	}

	uint8 system_upgrade_flag_check() {
		debug2(__LINE__);
		esp8266 *esp = getesp();
		return esp->upgrade;
	}

}
