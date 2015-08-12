#include "espsim.h"

static void debug2(int line) {
	//os_printf("%d\n", line);
}

extern "C" {
	bool wifi_set_opmode(uint8 mode) {
		debug2(__LINE__);
		return false;
	}

	bool wifi_softap_get_config(struct softap_config *config) {
		debug2(__LINE__);
		esp8266 *esp = getesp();
		os_memcpy(config, &esp->softap, sizeof(struct softap_config));
		return false;
	}

	sint8 wifi_station_get_rssi() {
		debug2(__LINE__);
		return 0;
	}

	bool wifi_station_dhcpc_start() {
		debug2(__LINE__);
		return true;
	}

	bool wifi_station_dhcpc_stop() {
		debug2(__LINE__);
		return true;
	}

	uint8 wifi_station_get_connect_status() {
		debug2(__LINE__);
		return STATION_GOT_IP;
	}

	uint8 wifi_station_get_auto_connect() {
		debug2(__LINE__);
		return false;
	}

	enum dhcp_status wifi_station_dhcpc_status() {
		debug2(__LINE__);
		return DHCP_STARTED;
	}

	uint8 wifi_get_opmode(void) {
		debug2(__LINE__);
		return 0;
	}

	uint8 wifi_get_opmode_default(void) {
		debug2(__LINE__);
		return 0;
	}

	bool wifi_set_opmode_current(uint8 opmode) {
		debug2(__LINE__);
		return 0;
	}

	uint8 wifi_get_broadcast_if(void) {
		debug2(__LINE__);
		return 0;
	}

	bool wifi_set_broadcast_if(uint8 interface) {
		debug2(__LINE__);
		return 0;
	}

	bool wifi_station_get_config(struct station_config *config) {
		debug2(__LINE__);
		esp8266 *esp = getesp();
		os_memcpy(config, &esp->station, sizeof(station_config));
		return 0;
	}

	bool wifi_station_get_config_default(struct station_config *config) {
		debug2(__LINE__);
		esp8266 *esp = getesp();
		os_memcpy(config, &esp->station, sizeof(station_config));
		return 0;
	}

	bool wifi_station_set_config(struct station_config *config) {
		debug2(__LINE__);
		esp8266 *esp = getesp();
		os_memcpy(&esp->station, config, sizeof(station_config));
		return 0;
	}

	bool wifi_station_set_config_current(struct station_config *config) {
		debug2(__LINE__);
		esp8266 *esp = getesp();
		os_memcpy(&esp->station, config, sizeof(station_config));
		return 0;
	}

	bool wifi_get_ip_info(uint8 if_index, struct ip_info *info) {
		debug2(__LINE__);

		esp8266 *esp = getesp();

		if (esp->ipinfo[0].ip.addr == 0)
			getipinfo(&esp->ipinfo[if_index].ip.addr, &esp->ipinfo[if_index].netmask.addr);

		os_memcpy(info, esp->ipinfo + if_index, sizeof(struct ip_info));

		return 0;
	}

	bool wifi_set_ip_info(uint8 if_index, struct ip_info *info) {
		debug2(__LINE__);
		esp8266 *esp = getesp();
		os_memcpy(esp->ipinfo + if_index, info, sizeof(struct ip_info));
		return 0;
	}

	bool wifi_get_macaddr(uint8 if_index, uint8 *macaddr) {
		debug2(__LINE__);
		esp8266 *esp = getesp();

		int test;
		memcpy(&test, esp->macaddr, sizeof(test));

		if (!test)
			getmacaddr(esp->macaddr);

		os_memcpy(macaddr, esp->macaddr, 6);
		return true;
	}

	bool wifi_set_macaddr(uint8 if_index, uint8 *macaddr) {
		debug2(__LINE__);
		esp8266 *esp = getesp();
		os_memcpy(esp->macaddr, macaddr, 6);
		return 0;
	}

	uint8 wifi_get_channel(void) {
		debug2(__LINE__);
		return 0;
	}
	bool wifi_set_channel(uint8 channel) {
		debug2(__LINE__);
		return 0;
	}

	void wifi_status_led_install(uint8 gpio_id, uint32 gpio_name, uint8 gpio_func) {
		debug2(__LINE__);
	}

	void wifi_status_led_uninstall() {
		debug2(__LINE__);
	}

	bool wifi_station_scan(struct scan_config *config, scan_done_cb_t cb) {
		debug2(__LINE__);
		return 0;
	}

	bool wifi_station_connect() {
		debug2(__LINE__);
		return 0;
	}

}
