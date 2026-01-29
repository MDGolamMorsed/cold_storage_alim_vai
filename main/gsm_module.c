/**
 * @file gsm_module.c
 * @brief Implementation of GSM module functions for A7670E
 */

#include "gsm_module.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static const char *TAG = "gsm_module";

#define GSM_UART_PORT CONFIG_GSM_UART_PORT_NUM
#define GSM_TX_PIN CONFIG_GSM_UART_TX_PIN
#define GSM_RX_PIN CONFIG_GSM_UART_RX_PIN
#define GSM_BAUD_RATE CONFIG_GSM_UART_BAUD_RATE
#define GSM_BUF_SIZE CONFIG_GSM_BUF_SIZE
#define EMERGENCY_NUMBER CONFIG_GSM_EMERGENCY_NUMBER
#define GSM_PWR_PIN 21
#define GSM_RST_PIN 20

/**
 * @brief Initialize the GSM module UART.
 */
esp_err_t gsm_module_init(void)
{
    uart_config_t uart_config = {
        .baud_rate = GSM_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_LOGI(TAG, "Initializing GSM UART...");
    ESP_ERROR_CHECK(uart_driver_install(GSM_UART_PORT, GSM_BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(GSM_UART_PORT, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(GSM_UART_PORT, GSM_TX_PIN, GSM_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    // Initialize Power and Reset pins
    ESP_LOGI(TAG, "Configuring GSM Power (GPIO %d) and Reset (GPIO %d)...", GSM_PWR_PIN, GSM_RST_PIN);
    gpio_reset_pin(GSM_PWR_PIN);
    gpio_reset_pin(GSM_RST_PIN);
    gpio_set_direction(GSM_PWR_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(GSM_RST_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(GSM_RST_PIN, 1); // Default High (Inactive)
    gpio_set_level(GSM_PWR_PIN, 1); // Default High (Inactive)

    // Reset and Power-on sequence
    ESP_LOGI(TAG, "Resetting and Powering on GSM module...");
    // 1. Reset (Active Low)
    gpio_set_level(GSM_RST_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(200));
    gpio_set_level(GSM_RST_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(2000)); // Wait for reset
    // 2. Power Key (Active Low)
    gpio_set_level(GSM_PWR_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(1500)); // Hold > 1s
    gpio_set_level(GSM_PWR_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(12000)); // Wait for boot (increased to avoid URC collisions)

    // Sync Baud Rate and Basic AT check
    ESP_LOGI(TAG, "Syncing GSM module...");
    int retry = 0;
    bool synced = false;
    while (retry < 20)
    {
        uart_flush_input(GSM_UART_PORT);
        uart_write_bytes(GSM_UART_PORT, "AT\r\n", 4);
        uint8_t rx_data[64] = {0};
        int len = uart_read_bytes(GSM_UART_PORT, rx_data, sizeof(rx_data) - 1, pdMS_TO_TICKS(500));
        if (len > 0)
        {
            ESP_LOGI(TAG, "Sync attempt %d response: %s", retry, (char *)rx_data);
            if (strstr((char *)rx_data, "OK"))
            {
                synced = true;
                break;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(500));
        retry++;
    }

    if (!synced)
        ESP_LOGE(TAG, "GSM Sync Failed! Check wiring/power.");

    // Disable Echo to simplify parsing and reduce garbage
    gsm_module_send_at_cmd("ATE0");
    // Enable verbose error messages
    gsm_module_send_at_cmd("AT+CMEE=2");

    // Set SMS text mode
    gsm_module_send_at_cmd("AT+CMGF=1");
    // Show SMS directly on UART
    gsm_module_send_at_cmd("AT+CNMI=2,2,0,0,0");

    return ESP_OK;
}

/**
 * @brief Send AT command and print reply.
 */
esp_err_t gsm_module_send_at_cmd(const char *cmd)
{
    if (cmd == NULL)
        return ESP_ERR_INVALID_ARG;

    // Flush input buffer to clear previous data/noise
    uart_flush_input(GSM_UART_PORT);

    char tx_buf[128];
    snprintf(tx_buf, sizeof(tx_buf), "%s\r\n", cmd);

    int len = uart_write_bytes(GSM_UART_PORT, tx_buf, strlen(tx_buf));
    if (len < 0)
    {
        ESP_LOGE(TAG, "UART write failed");
        return ESP_FAIL;
    }

    // Read response to print it
    uint8_t *data = (uint8_t *)malloc(GSM_BUF_SIZE);
    if (data)
    {
        memset(data, 0, GSM_BUF_SIZE);
        int rx_len = uart_read_bytes(GSM_UART_PORT, data, GSM_BUF_SIZE - 1, pdMS_TO_TICKS(2000));
        if (rx_len > 0)
        {
            data[rx_len] = 0;
            ESP_LOGI(TAG, "AT CMD Reply for %s:\n%s", cmd, (char *)data);
        }
        else
        {
            ESP_LOGW(TAG, "No response for command: %s", cmd);
        }
        free(data);
    }

    return ESP_OK;
}

/**
 * @brief Send SMS.
 */
esp_err_t gsm_module_send_sms(const char *phone_number, const char *message)
{
    char cmd_buf[64];
    snprintf(cmd_buf, sizeof(cmd_buf), "AT+CMGS=\"%s\"", phone_number);

    gsm_module_send_at_cmd(cmd_buf);
    vTaskDelay(pdMS_TO_TICKS(100));

    // Send message body + Ctrl+Z (0x1A)
    char msg_buf[256];
    snprintf(msg_buf, sizeof(msg_buf), "%s%c", message, 0x1A);
    uart_write_bytes(GSM_UART_PORT, msg_buf, strlen(msg_buf));

    ESP_LOGI(TAG, "SMS Sent to %s: %s", phone_number, message);
    return ESP_OK;
}

/**
 * @brief Publish MQTT message.
 */
esp_err_t gsm_module_mqtt_publish(const char *topic, const char *payload)
{
    char cmd[128];

    // Set Topic
    snprintf(cmd, sizeof(cmd), "AT+CMQTTTOPIC=0,%d", strlen(topic));
    gsm_module_send_at_cmd(cmd);
    uart_write_bytes(GSM_UART_PORT, topic, strlen(topic));
    vTaskDelay(pdMS_TO_TICKS(100));

    // Set Payload
    snprintf(cmd, sizeof(cmd), "AT+CMQTTPAYLOAD=0,%d", strlen(payload));
    gsm_module_send_at_cmd(cmd);
    uart_write_bytes(GSM_UART_PORT, payload, strlen(payload));
    vTaskDelay(pdMS_TO_TICKS(100));

    // Publish
    gsm_module_send_at_cmd("AT+CMQTTPUB=0,1,60");

    return ESP_OK;
}

/**
 * @brief Call emergency number.
 */
esp_err_t gsm_module_call_emergency(void)
{
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "ATD%s;", EMERGENCY_NUMBER);
    ESP_LOGI(TAG, "Calling emergency number: %s", EMERGENCY_NUMBER);
    return gsm_module_send_at_cmd(cmd);
}

/**
 * @brief Process incoming UART data.
 */
void gsm_module_process_data(float *temp_threshold, float *hum_threshold)
{
    size_t buffered_len;
    uart_get_buffered_data_len(GSM_UART_PORT, &buffered_len);

    if (buffered_len > 0)
    {
        uint8_t *data = (uint8_t *)malloc(buffered_len + 1);
        int len = uart_read_bytes(GSM_UART_PORT, data, buffered_len, pdMS_TO_TICKS(20));
        if (len > 0)
        {
            data[len] = 0;
            char *str_data = (char *)data;

            // Print if ring is ongoing
            if (strstr(str_data, "RING"))
            {
                ESP_LOGI(TAG, "Incoming Call: Ring is ongoing...");
            }

            // Check for Threshold commands
            char *temp_ptr = strstr(str_data, "TEMP_TH:");
            if (temp_ptr)
            {
                float val = atof(temp_ptr + 8);
                if (val > 0)
                    *temp_threshold = val;
            }

            char *hum_ptr = strstr(str_data, "HUM_TH:");
            if (hum_ptr)
            {
                float val = atof(hum_ptr + 7);
                if (val > 0)
                    *hum_threshold = val;
            }
        }
        free(data);
    }
}