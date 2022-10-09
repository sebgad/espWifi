#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "wifi.h"
#include <string>
#include <iostream>
#include "nvs_flash.h"

Wifi::state_e objWifiState = Wifi::state_e::NOT_INITIALIZED;
Wifi objWifi;

// necessary for cpp
extern "C" {
  void app_main();
}

void app_main(void)
{
    // Create default system event loops, e.g. for wifi events
    esp_event_loop_create_default();
    // initialize non-volatile memory
    nvs_flash_init();

    // set credentials for wifi
    objWifi.SetCredentials("FRITZ!Box_SY_2_4", "992979374447774325495525");
    // initialize wifi
    objWifi.Init();

    while(1){
        objWifiState = objWifi.GetState();

        switch (objWifiState)
        {
        case Wifi::state_e::READY_TO_CONNECT:
            std::cout << "Wifi Status: READY_TO_CONNECT\n";
            objWifi.Begin();
            break;
        case Wifi::state_e::DISCONNECTED:
            std::cout << "Wifi Status: DISCONNECTED\n";
            objWifi.Begin();
            break;
        case Wifi::state_e::CONNECTING:
            std::cout << "Wifi Status: CONNECTING\n";
            break;
        case Wifi::state_e::WAITING_FOR_IP:
            std::cout << "Wifi Status: WAITING_FOR_IP\n";
            break;
        case Wifi::state_e::ERROR:
            std::cout << "Wifi Status: ERROR\n";
            break;
        case Wifi::state_e::CONNECTED:
            std::cout << "Wifi Status: CONNECTED\n";
            break;
        case Wifi::state_e::NOT_INITIALIZED:
            std::cout << "Wifi Status: NOT_INITIALIZED\n";
            break;
        case Wifi::state_e::INITIALIZED:
            std::cout << "Wifi Status: INITIALIZED\n";
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
