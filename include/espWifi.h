// inspired by https://github.com/espressif/esp-idf/tree/fa5d0a351343b45ee06d221b7c29028672a4c3c2/examples/wifi/getting_started/station

#include <cmath>
#include <cstring>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#define strLogTag "espWifi"

#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif

#define WIFI_SSID CONFIG_ESP_WIFI_SSID
#define WIFI_PW CONFIG_ESP_WIFI_PASSWORD

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

class espWifi
{
public:

private:
    static void _init();
    static EventGroupHandle_t _objWifiGrpHdle;

    static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
    static void ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

    static int _iRetryNumAtt;

public:
    espWifi(void);

    void SetCredentials(const char *ssid, const char *password);
    void Init();
    esp_err_t Begin(void);
};