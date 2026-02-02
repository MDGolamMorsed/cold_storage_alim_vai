#include "network.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "sdkconfig.h"
#include "app_config.h"
#ifdef CONFIG_CONNECTION_TYPE_WIFI
#include <wifi_provisioning/manager.h>
#include <wifi_provisioning/scheme_softap.h>
// Explicitly declare the handler to resolve visibility issues
extern void wifi_prov_scheme_softap_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
#endif
#include <stdio.h>

static const char *TAG = "WIFI_NETWORK";
static esp_mqtt_client_handle_t mqtt_client = NULL;
static bool mqtt_started = false;
static bool is_connected = false;

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        wifi_event_sta_disconnected_t *event = (wifi_event_sta_disconnected_t *)event_data;
        is_connected = false;
        ESP_LOGW(TAG, "WiFi Disconnected (Reason: %d). Retrying...", event->reason);
        esp_wifi_connect();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        is_connected = true;
        ESP_LOGI(TAG, "WiFi Connected");
#if defined(CONFIG_CONNECTION_TYPE_WIFI) && defined(CONFIG_ENABLE_MQTT)
        if (mqtt_client && !mqtt_started)
        {
            esp_mqtt_client_start(mqtt_client);
            mqtt_started = true;
            ESP_LOGI(TAG, "MQTT Client started");
        }
#endif
    }
}

#ifdef CONFIG_CONNECTION_TYPE_WIFI
static void provisioning_event_handler(void *arg, esp_event_base_t event_base,
                                       int32_t event_id, void *event_data)
{
    if (event_base == WIFI_PROV_EVENT)
    {
        switch (event_id)
        {
        case WIFI_PROV_START:
            ESP_LOGI(TAG, "Provisioning started");
            break;
        case WIFI_PROV_CRED_RECV:
        {
            wifi_sta_config_t *wifi_sta_cfg = (wifi_sta_config_t *)event_data;
            ESP_LOGI(TAG, "Received Wi-Fi credentials"
                          "\n\tSSID     : %s"
                          "\n\tPassword : ******",
                     (const char *)wifi_sta_cfg->ssid);
            break;
        }
        case WIFI_PROV_CRED_FAIL:
        {
            wifi_prov_sta_fail_reason_t *reason = (wifi_prov_sta_fail_reason_t *)event_data;
            ESP_LOGE(TAG, "Provisioning failed!\n\tReason : %s"
                          "\n\tPlease reset to factory and retry provisioning",
                     (*reason == WIFI_PROV_STA_AUTH_ERROR) ? "Wi-Fi station authentication failed" : "Wi-Fi access-point not found");
            break;
        }
        case WIFI_PROV_CRED_SUCCESS:
            ESP_LOGI(TAG, "Provisioning successful");
            break;
        case WIFI_PROV_END:
            wifi_prov_mgr_deinit();
            break;
        default:
            break;
        }
    }
}
#endif

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "MQTT Event dispatched: %d", (int)event_id);
}

void network_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

#ifdef CONFIG_CONNECTION_TYPE_WIFI
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip));

    // Initialize Provisioning Manager
    wifi_prov_mgr_config_t config = {
        .scheme = &wifi_prov_scheme_softap,
        .scheme_event_handler = wifi_prov_scheme_softap_event_handler};
    ESP_ERROR_CHECK(wifi_prov_mgr_init(config));

    bool provisioned = false;
    ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));

    if (!provisioned)
    {
        ESP_LOGI(TAG, "Starting provisioning...");
        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &provisioning_event_handler, NULL));

        char service_name[12];
        uint8_t eth_mac[6];
        esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
        snprintf(service_name, sizeof(service_name), "PROV_%02X%02X%02X", eth_mac[3], eth_mac[4], eth_mac[5]);

        wifi_prov_security_t security = WIFI_PROV_SECURITY_1;
        const char *pop = CONFIG_WIFI_PROV_POP;

        // Start provisioning service
        wifi_prov_mgr_start_provisioning(security, pop, service_name, NULL);
    }
    else
    {
        ESP_LOGI(TAG, "Already provisioned, starting Wi-Fi...");
        wifi_prov_mgr_deinit();
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_start());
    }
#endif

#if defined(CONFIG_CONNECTION_TYPE_WIFI) && defined(CONFIG_ENABLE_MQTT) 
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = CONFIG_MQTT_BROKER_URL,
    };
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    // MQTT Client will be started in GOT_IP event
#endif
}

void network_send_data(const sensor_readings_t *readings)
{
#if defined(CONFIG_CONNECTION_TYPE_WIFI) && defined(CONFIG_ENABLE_MQTT) 

    if (!mqtt_client || !is_connected)
    {
        ESP_LOGW(TAG, "Cannot send MQTT: Not connected");
        return;
    }

    char payload[128];
    snprintf(payload, sizeof(payload),
             "{\"dht_temp\": %.2f, \"dht_hum\": %.2f, \"ds_temp\": %.2f}",
             readings->dht_temp, readings->dht_humidity, readings->ds_temp);

    int msg_id = esp_mqtt_client_publish(mqtt_client, mqtt_pub_topic, payload, 0, 1, 0);
    ESP_LOGI(TAG, "MQTT Sent: %s, ID: %d", payload, msg_id);
#else
    ESP_LOGI(TAG, "MQTT Disabled. Data not sent.");
#endif
}

int network_is_connected(void)
{
    return is_connected;
}