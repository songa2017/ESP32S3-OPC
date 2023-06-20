#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"

#define WIFI_SSID "ssid"
#define WIFI_PASSWORD "password"
void wifi_init_sta(void);
bool wifi_get_status();
esp_netif_ip_info_t* wifi_get_ip_info();