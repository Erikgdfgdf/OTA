#include "ota.h"
#include "esp_ota_ops.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_https_ota.h"
#include "github_ca_pem.h"
#include "version.h"

static const char* TAG = "OTA";

#define OTA_URL "https://raw.githubusercontent.com/Erikgdfgdf/OTA/refs/heads/main/OTA-ESP32/server/firmware.bin"
#define FIRMWARE_JSON_URL "https://raw.githubusercontent.com/Erikgdfgdf/OTA/refs/heads/main/OTA-ESP32/server/firmware.json"

#define CURRENT_VERSION FIRMWARE_VERSION
#define JSON_BUFFER_SIZE 512

static bool extract_json_string(const char* json, const char* key, char* out, size_t out_size) {
    // Build the search pattern: "key":"
    char search[64];
    snprintf(search, sizeof(search), "\"%s\": \"", key);

    // Find the key in the JSON string
    const char* start = strstr(json, search);
    if (!start) return false;

    // Move past the key and opening quote to the value
    start += strlen(search);

    // Find the closing quote of the value
    const char* end = strchr(start, '"');
    if (!end) return false;

    // Copy the value into the output buffer
    size_t len = end - start;
    if (len >= out_size) len = out_size - 1;
    strncpy(out, start, len);
    out[len] = '\0';

    return true;
}

static esp_err_t fetch_latest_version(char* out_version, char* out_url, size_t buf_size){
    char response[JSON_BUFFER_SIZE] = {0};

    esp_http_client_config_t config = {};
    config.url = FIRMWARE_JSON_URL;
    config.cert_pem = (const char*)github_ca_pem;
    
    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return err;
    }

    int content_length = esp_http_client_fetch_headers(client);
    ESP_LOGI(TAG, "Content-Length: %d", content_length);

    int read_len= esp_http_client_read(client, response, sizeof(response) - 1);
    ESP_LOGI(TAG, "Read %d bytes: %s", read_len, response);

   esp_http_client_close(client);
    esp_http_client_cleanup(client);

    if (read_len <= 0) {
        ESP_LOGE(TAG, "Empty response body");
        return ESP_FAIL;
    }

    if (!extract_json_string(response, "version", out_version, buf_size)) {
        ESP_LOGE(TAG, "Could not find 'version' in JSON");
        return ESP_FAIL;
    }

    if (!extract_json_string(response, "url", out_url, buf_size)) {
        ESP_LOGW(TAG, "No 'url' in JSON, using default");
        strncpy(out_url, OTA_URL, buf_size - 1);
    }

    return ESP_OK;
}

void mark_firmware_valid(){
    esp_ota_mark_app_valid_cancel_rollback();
    ESP_LOGI(TAG, "Firmware marked as valid");
}

bool ota_check_and_update() {
    char latest_version[32] = {0};
    char firmware_url[512] = {0};

    ESP_LOGI(TAG, "Current firmware version: %s", CURRENT_VERSION);
    ESP_LOGI(TAG, "Checking for updates...");

    esp_err_t err = fetch_latest_version(latest_version, firmware_url, sizeof(firmware_url));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to fetch latest version info");
        return false;
    }

    ESP_LOGI(TAG, "Latest firmware version: %s", latest_version);

    if(strcmp(latest_version, CURRENT_VERSION) == 0) {
        ESP_LOGI(TAG, "Device is up to date");
        return false;
    }

    ESP_LOGI(TAG, "New firmware found: %s. Starting OTA update from %s", latest_version, firmware_url);

    esp_http_client_config_t http_config = {};
    http_config.url = firmware_url;
    http_config.cert_pem = (const char*)github_ca_pem;

    esp_https_ota_config_t ota_client_config = {};
    ota_client_config.http_config = &http_config;

    esp_err_t ota_err= esp_https_ota(&ota_client_config);

    if (ota_err == ESP_OK) {
        ESP_LOGI(TAG, "OTA update successful. Restarting...");
        return true;
    } else {
        ESP_LOGE(TAG, "OTA update failed: %s", esp_err_to_name(ota_err));
        return false;
    }
}

void ota_task(void* pvParameters) {
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(10000));  // check every 10 seconds
        bool updated = ota_check_and_update();
        if (updated){
             esp_restart();
        }
    }
}