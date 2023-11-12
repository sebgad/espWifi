set(SOURCES main.cpp
            ../espWifi.cpp)

idf_component_register(SRCS ${SOURCES}
                       INCLUDE_DIRS ../include
                       REQUIRES esp_wifi nvs_flash esp_event esp_netif lwip espWifi)
