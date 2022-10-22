#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "espWifi.h"
#include <string>
#include <iostream>
#include "nvs_flash.h"

espWifi objWifi;

// necessary for cpp
extern "C" {
  void app_main();
}

void app_main(void)
{
    // initialize non-volatile memory
    esp_err_t ret = nvs_flash_init();
    
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    
    ESP_ERROR_CHECK(ret);

    // set credentials for wifi
    objWifi.Init();
}
