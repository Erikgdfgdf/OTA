#include "ota.h"
#include "esp_ota_ops.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_https_ota.h"

static const char* TAG = "OTA";

#define OTA_URL "https://raw.githubusercontent.com/Erikgdfgdf/OTA/refs/heads/main/OTA-ESP32/server/firmware.bin"
#define FIRMWARE_JSON_URL "https://raw.githubusercontent.com/Erikgdfgdf/OTA/refs/heads/main/OTA-ESP32/server/firmware.json"

#define CURRENT_VERSION "1.0.0"
#define JSON_BUFFER_SIZE 512

extermal const char github_ca_pem_start[] asm("_binary_github_ca_pem_start");
extermal const char github_ca_pem_end[] asm("_binary_github_ca_pem");


static bool extract_json_string(const char* json, const char* key, char* out, size_t out_size) {
    // Build the search pattern: "key":"
    char search[64];
    snprintf(search, sizeof(search), "\"%s\":\"", key);

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
    config.cert_pem = github_ca_pem_start;

    esp_http_client_handle_t client = esp_http_client_init(&config);

    if (esp_http_client_perform(client) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(esp_http_client_get_status_code(client)));
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    esp_http_client_fetch_headers(client);

    esp_http_client_read(client, response, sizeof(response) - 1);
    esp_http_client_cleanup(client);

    ESP_LOGI(TAG, "firmware.json: %s", response);


    if (!extract_json_string(response, "version", out_version, buf_size)) {
        ESP_LOGE(TAG, "Could not find 'version' in JSON");
        return ESP_FAIL;
    }

    if (!extract_json_string(response, "url", out_url, buf_size)) {
        ESP_LOGE(TAG, "Could not find 'url' in JSON");
        return ESP_FAIL;
    }

    return ESP_OK;
}

void mark_firmware_valid(){
    esp_ota_mark_app_valid_cancel_rollback();
    ESP_LOGI(TAG, "Firmware marked as valid");
}

bool ota_check_and_update() {
    char latest_version[32] = {0};
    char firmware_url[256] = {0};

    ESP_LOGI(TAG, "Current firmware version: %s", CURRENT_VERSION);
    ESP_LOGI(TAG, "Checking for updates...");

    esp_err_t err = fetch_latest_version(latest_version, firmware_url, sizeof(latest_version));
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
    http_config.cert_pem = github_ca_pem_start;

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