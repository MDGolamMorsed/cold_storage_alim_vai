/**
 * @file gsm_module.h
 * @brief Modular GSM driver for A7670E module on ESP32-S3
 */

#pragma once

#include "esp_err.h"
#include <stdint.h>
#include "sensors.h"


#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Initialize the GSM module UART and basic settings.
     *
     * @return esp_err_t ESP_OK on success, ESP_FAIL otherwise.
     */
    esp_err_t gsm_module_init();

    /**
     * @brief Send an AT command to the GSM module and print the reply.
     *
     * @param cmd The AT command string (e.g., "AT+CPIN?").
     * @return esp_err_t ESP_OK if command sent, ESP_FAIL otherwise.
     */
    // esp_err_t gsm_module_send_at_cmd(const char *cmd);

    /**
     * @brief Send an SMS message.
     *
     * @param phone_number The target phone number.
     * @param message The message content.
     * @return esp_err_t ESP_OK on success.
     */
    esp_err_t gsm_module_send_sms(const char *message);

    /**
     * @brief Publish a message via MQTT (AT command wrapper).
     *
     * @param topic MQTT topic.
     * @param payload Message payload.
     * @return esp_err_t ESP_OK on success.
     */
    esp_err_t gsm_module_mqtt_publish(const sensor_readings_t *payload);

    /**
     * @brief Publish an alert message via MQTT.
     *
     * @param message Alert message string.
     * @return esp_err_t ESP_OK on success.
     */
    esp_err_t gsm_module_send_alert(const char *message);

    /**
     * @brief Initiate a voice call to the configured emergency number.
     *
     * @return esp_err_t ESP_OK on success.
     */
    esp_err_t gsm_module_call_emergency(void);

    /**
     * @brief Check for incoming data (SMS, MQTT, RING) and parse thresholds.
     *
     * @param temp_threshold Pointer to the temperature threshold variable to update.
     * @param hum_threshold Pointer to the humidity threshold variable to update.
     * @param readings Pointer to the latest sensor readings.
     */
    void gsm_module_process_data(float *temp_threshold, float *hum_threshold, const sensor_readings_t *readings);

#ifdef __cplusplus
}
#endif