#include "ota.h"
#include "esp_ota_ops.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_https_ota.h"

static const char* TAG = "OTA";

// edit the URL
#define OTA_URL "http://example.com/firmware.bin"


void ota_update() {
    ESP_LOGI(TAG, "Starting OTA update from URL: %s", OTA_URL);

    esp_http_client_config_t http_config = {};

    http_config.url = OTA_URL;

    esp_https_ota_config_t ota_config = {};

    ota_config.http_config = &http_config;

    esp_err_t ret = esp_https_ota(&ota_config);

    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "OTA successful. Restarting...");
        esp_restart();
    }
    else
    {
        ESP_LOGE(TAG, "OTA failed");
    }
}