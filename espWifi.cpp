#include "espWifi.h"

// Define Statics for referencing (necessary for code compilation)
EventGroupHandle_t espWifi::_objWifiGrpHdle;
int espWifi::_iRetryNumAtt = 0;
volatile enum e_WifiState eStateWifi = WIFI_INIT;

// Wifi Constructor
espWifi::espWifi(void)
{
}

void espWifi::wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT) {
        const wifi_event_t event_type = static_cast<wifi_event_t>(event_id);

        switch (event_type) {
        case WIFI_EVENT_STA_START: {
            esp_wifi_connect();
            break;
        }

        case WIFI_EVENT_STA_CONNECTED: {
            break;
        }

        case WIFI_EVENT_STA_DISCONNECTED: {
            if (_iRetryNumAtt <= CONFIG_ESP_WIFI_MAXIMUM_RETRY){
                esp_wifi_connect();
                ESP_LOGI(strLogTag, "retry %d of %d to connect to the AP", _iRetryNumAtt, CONFIG_ESP_WIFI_MAXIMUM_RETRY);
                _iRetryNumAtt++;
            } else {
                xEventGroupSetBits(_objWifiGrpHdle, WIFI_FAIL_BIT);
                ESP_LOGW(strLogTag, "wifi connection failed. Try to set soft AP");
            }
            break;
        }

        default:
            break;
        }
    }
}

void espWifi::ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == IP_EVENT) {
        const ip_event_t event_type = static_cast<ip_event_t>(event_id);

        switch (event_type) {
        case IP_EVENT_STA_GOT_IP: {
            ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
            ESP_LOGI(strLogTag, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
            _iRetryNumAtt = 0;
            xEventGroupSetBits(_objWifiGrpHdle, WIFI_CONNECTED_BIT);
            break;
        }

        default:
            break;
        }
    }
}

void espWifi::_init() {
    /**
     * @brief Initialize wifi connection, default STA mode
     * 
     * @return  NONE
     */

    _objWifiGrpHdle = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_LOGI(strLogTag, "netif initialized correctly.");

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_LOGI(strLogTag, "Default Event loops are created.");

    const esp_netif_t *const p_netif = esp_netif_create_default_wifi_sta();
    assert(p_netif);

    wifi_init_config_t obj_wifi_init_cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_wifi_init(&obj_wifi_init_cfg));
    
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, nullptr, nullptr));
    ESP_LOGI(strLogTag, "Wifi event handler registered successfully.");

    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &ip_event_handler, nullptr, nullptr));
    ESP_LOGI(strLogTag, "IP event handler registered successfully.");

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_LOGI(strLogTag, "Set wifi mode to Station successfully.");

    // set auth mode settings for sta when previous step run successfully.
    wifi_config_t obj_wifi_cfg = {};
    obj_wifi_cfg.sta.threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD;
    obj_wifi_cfg.sta.pmf_cfg.capable = true;
    obj_wifi_cfg.sta.pmf_cfg.required = false;
    memcpy(obj_wifi_cfg.sta.ssid, CONFIG_ESP_WIFI_SSID, std::min(strlen(CONFIG_ESP_WIFI_SSID), sizeof(obj_wifi_cfg.sta.ssid)));
    memcpy(obj_wifi_cfg.sta.password, CONFIG_ESP_WIFI_PASSWORD, std::min(strlen(CONFIG_ESP_WIFI_PASSWORD), sizeof(obj_wifi_cfg.sta.password)));

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &obj_wifi_cfg));
    ESP_LOGI(strLogTag, "Set auth mode settings for Station successfully.");
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(strLogTag, "wifi initialized successfully, ready for connecting.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(_objWifiGrpHdle,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);


    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(strLogTag, "connected to ap SSID:%s password:%s",
                 CONFIG_ESP_WIFI_SSID, CONFIG_ESP_WIFI_PASSWORD);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(strLogTag, "Failed to connect to SSID:%s, password:%s",
                 CONFIG_ESP_WIFI_SSID, CONFIG_ESP_WIFI_PASSWORD);
    } else {
        ESP_LOGE(strLogTag, "UNEXPECTED EVENT");
    }
}

void espWifi::Init()
{
    return _init();
}