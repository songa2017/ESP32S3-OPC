#include <stdio.h>
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "configure.h"
#include "wifi/wifi.h"
#ifdef USE_OPC
#include "opc/opc.h"
#endif
void app_main(void)
{
	//Initialize NVS
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	ESP_LOGI("WIFI", "ESP_WIFI_MODE_STA");
	wifi_init_sta();
	if (!wifi_get_status())
	{
		vTaskDelay(2000);
	}
	
	esp_netif_ip_info_t* wifi_ip_info = wifi_get_ip_info();
	if (wifi_ip_info)
	{
		printf("My IP: " IPSTR "\n", IP2STR(&wifi_ip_info->ip));
		printf("My GW: " IPSTR "\n", IP2STR(&wifi_ip_info->gw));
		printf("My NETMASK: " IPSTR "\n", IP2STR(&wifi_ip_info->netmask));		
#ifdef USE_OPC
	
		opc_task();
#endif
	}
	

}