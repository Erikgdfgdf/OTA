#include "wifi.h"
#include "ota.h"
#include <iostream>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define RED_PIN GPIO_NUM_12
#define GREEN_PIN GPIO_NUM_14

//#define LED_PIN RED_PIN
#define LED_PIN GREEN_PIN


void led_init() {
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << RED_PIN) | (1ULL << GREEN_PIN);
    io_conf.mode = GPIO_MODE_OUTPUT;
    gpio_config(&io_conf);
    
    gpio_set_level(RED_PIN, 0);
    gpio_set_level(GREEN_PIN, 0);
}

extern "C" void app_main(void)
{
    led_init();

    std::cout << "Starting WiFi connection..." << std::endl;

    wifi_init();

    if (wifi_is_connected())
    {
        mark_firmware_valid();
    } else {
        ESP_LOGE("MAIN", "WiFi not connected- bootloader will rollback"); 
    }
    
    //added task to check for updates every 10 seconds
    xTaskCreate(ota_task, "ota_task", 8192, NULL, 5, NULL);
   
      while (true) {
          
        gpio_set_level(LED_PIN,1);
        vTaskDelay(pdMS_TO_TICKS(500));
        gpio_set_level(LED_PIN,0);
        vTaskDelay(pdMS_TO_TICKS(500));
      }
   
}

