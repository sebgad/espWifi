#include "wifi.h"

// Statics
char Wifi::_mac_addr_cstr[]{};
std::mutex Wifi::_mutx{};
Wifi::state_e Wifi::_state{state_e::NOT_INITIALIZED};
wifi_init_config_t Wifi::_wifi_init_cfg = WIFI_INIT_CONFIG_DEFAULT();
wifi_config_t Wifi::_wifi_cfg{};

// Wifi Constructor
Wifi::Wifi(void)
{
    if (!_mac_addr_cstr[0])
    {
        if (ESP_OK != _get_mac())
        {
            ESP_LOGE(strLogTag, "MAC Address not received corretly. Restart ESP.");
            esp_restart();
        }
    }
}

void Wifi::wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT) {
        const wifi_event_t event_type = static_cast<wifi_event_t>(event_id);

        switch (event_type) {
        case WIFI_EVENT_STA_START: {
            std::lock_guard<std::mutex> state_guard(_mutx);
            _state = state_e::READY_TO_CONNECT;
            break;
        }

        case WIFI_EVENT_STA_CONNECTED: {
            std::lock_guard<std::mutex> state_guard(_mutx);
            _state = state_e::WAITING_FOR_IP;
            break;
        }

        case WIFI_EVENT_STA_DISCONNECTED: {
            std::lock_guard<std::mutex> state_guard(_mutx);
            _state = state_e::DISCONNECTED;
            break;
        }

        default:
            break;
        }
    }
}

void Wifi::ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == IP_EVENT) {
        const ip_event_t event_type = static_cast<ip_event_t>(event_id);

        switch (event_type) {
        case IP_EVENT_STA_GOT_IP: {
            std::lock_guard<std::mutex> state_guard(_mutx);
            ESP_LOGI(strLogTag, "IP address received.");
            _state = state_e::CONNECTED;
            break;
        }

        case IP_EVENT_STA_LOST_IP: {
            std::lock_guard<std::mutex> state_guard(_mutx);
            if (_state != state_e::DISCONNECTED)
            {
                ESP_LOGW(strLogTag, "Lost IP. Waiting for IP.");
                _state = state_e::WAITING_FOR_IP;
            }
            break;
        }

        default:
            break;
        }
    }
}

esp_err_t Wifi::Begin(void)
{
    std::lock_guard<std::mutex> connect_guard(_mutx);

    esp_err_t status = ESP_OK;

    switch (_state) {
    case state_e::READY_TO_CONNECT:
    case state_e::DISCONNECTED:
        status = esp_wifi_connect();
        if (ESP_OK == status)
        {
            _state = state_e::CONNECTING;
        }
        break;
    case state_e::CONNECTING:
    case state_e::WAITING_FOR_IP:
    case state_e::CONNECTED:
        break;
    case state_e::NOT_INITIALIZED:
    case state_e::INITIALIZED:
    case state_e::ERROR:
        status = ESP_FAIL;
        break;
    }
    return status;
}

esp_err_t Wifi::_init() {
    /**
     * @brief Initialize wifi connection, default STA mode
     * 
     * @return  obj_err_status: Error status of connection
     */

    
    
    // Mutex lock to prevent changing wifi settings with different threads at the same time
    std::lock_guard<std::mutex> mutx_guard(_mutx);
    
    // initialize error status
    esp_err_t obj_err_status = ESP_OK;

    if (state_e::NOT_INITIALIZED == _state) {
        // start establishing connection when not initialized

        obj_err_status |= esp_netif_init();

        if (obj_err_status != ESP_OK) {
            // not initialized correctly.
            ESP_LOGW(strLogTag, "netif already initialized, try to initialize it again.");
            if (esp_netif_deinit() == ESP_OK) {
                obj_err_status = esp_netif_init();
                if ( obj_err_status == ESP_OK) {
                    ESP_LOGI(strLogTag, "netif reinitialized successful.");
                } else {
                    ESP_LOGI(strLogTag, "netif not reinitialized successful. abort wifi connection.");
                }
            }
        } else {
            ESP_LOGI(strLogTag, "netif initialized correctly.");
        };

        if (obj_err_status == ESP_OK) {
            // only create default wifi sta when netif successful.
            const esp_netif_t *const p_netif = esp_netif_create_default_wifi_sta();

            if (!p_netif) {
                obj_err_status = ESP_FAIL;
                ESP_LOGW(strLogTag, "User init default station not successful.");
            } else {
                ESP_LOGI(strLogTag, "User init default station successful.");
            }
        }

        if (obj_err_status == ESP_OK) {
            // init esp wifi, when previous step run successfully.
            obj_err_status = esp_wifi_init(&_wifi_init_cfg);
            if (obj_err_status == ESP_OK) {
                ESP_LOGI(strLogTag, "Wifi init settings set successfully.");
            } else {
                ESP_LOGW(strLogTag, "Wifi init settings not set successfully.");
            }

        }

        if (obj_err_status == ESP_OK) {
            // register wifi event handler when previous step run successfully.
            obj_err_status = esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, nullptr, nullptr);
            if (obj_err_status == ESP_OK) {
                ESP_LOGI(strLogTag, "Wifi event handler registered successfully.");
            } else {
                ESP_LOGW(strLogTag, "Wifi event handler not registered successfully.");
            }
        }

        if (obj_err_status == ESP_OK) {
            // register ip_event handler when previous step run successfully.
            obj_err_status = esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &ip_event_handler, nullptr, nullptr);
            if (obj_err_status == ESP_OK) {
                ESP_LOGI(strLogTag, "IP event handler registered successfully.");
            } else {
                ESP_LOGW(strLogTag, "IP event handler not registered successfully.");
            }
        }

        if (obj_err_status == ESP_OK) {
            // set wifi mode when previous step run successfully.
            obj_err_status = esp_wifi_set_mode(WIFI_MODE_STA);
            if (obj_err_status == ESP_OK) {
                ESP_LOGI(strLogTag, "Set wifi mode to Station successfully.");
            } else {
                ESP_LOGW(strLogTag, "Set wifi mode to Station not successfully.");
            }

        }

        if (ESP_OK == obj_err_status) {
            // set auth mode settings for sta when previous step run successfully.
            _wifi_cfg.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
            _wifi_cfg.sta.pmf_cfg.capable = true;
            _wifi_cfg.sta.pmf_cfg.required = false;

            obj_err_status = esp_wifi_set_config(WIFI_IF_STA, &_wifi_cfg);
            if (obj_err_status == ESP_OK) {
                ESP_LOGI(strLogTag, "Set auth mode settings for Station successfully.");
            } else {
                ESP_LOGW(strLogTag, "Set auth mode settings for Station not successfully.");
            }
        }

        if (ESP_OK == obj_err_status) {
            // start wifi when previous step run successfully.
            obj_err_status = esp_wifi_start();
        }

        if (ESP_OK == obj_err_status) {
            // everything runs smooth
            ESP_LOGI(strLogTag, "wifi initialized successfully, ready for connecting.");
            _state = state_e::INITIALIZED;
        } else {
            ESP_LOGW(strLogTag, "Wifi not initialized successfully.");
        }
    } else if (_state == state_e::ERROR) {
        // set state to not initialized, when error
        ESP_LOGW(strLogTag, "Error state in Wifi, set back to NOT_INITIALIZED.");
        _state = state_e::NOT_INITIALIZED;
    }

    return obj_err_status;
}

void Wifi::SetCredentials(const char *ssid, const char *password)
{
    memcpy(_wifi_cfg.sta.ssid, ssid, std::min(strlen(ssid), sizeof(_wifi_cfg.sta.ssid)));

    memcpy(_wifi_cfg.sta.password, password, std::min(strlen(password), sizeof(_wifi_cfg.sta.password)));
}

esp_err_t Wifi::Init()
{
    return _init();
}

// Get default MAC from API and convert to ASCII HEX
esp_err_t Wifi::_get_mac(void)
{
    uint8_t mac_byte_buffer[6]{};

    const esp_err_t status{esp_efuse_mac_get_default(mac_byte_buffer)};

    if (ESP_OK == status)
    {
        snprintf(_mac_addr_cstr, sizeof(_mac_addr_cstr), "%02X%02X%02X%02X%02X%02X",
                    mac_byte_buffer[0],
                    mac_byte_buffer[1],
                    mac_byte_buffer[2],
                    mac_byte_buffer[3],
                    mac_byte_buffer[4],
                    mac_byte_buffer[5]);
    }

    return status;
}