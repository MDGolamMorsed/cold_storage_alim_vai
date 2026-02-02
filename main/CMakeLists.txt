idf_component_register(SRCS "app_config.c" "gsm_module.c" "main.c" "sensors.c" "network.c" "dht_wrapper.c" "ds18b20_wrapper.c"
                       INCLUDE_DIRS "."
                       REQUIRES nvs_flash esp_wifi esp_event mqtt driver wifi_provisioning esp_modem esp_netif)