#include "wifi.h"
#include "ota.h"
#include <iostream>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"


#define LED_PIN GPIO_NUM_4


void led_init() {

    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << LED_PIN);
    io_conf.mode = GPIO_MODE_OUTPUT;
    gpio_config(&io_conf);
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
    

    bool updated = ota_check_and_update();
    if (updated) {
        esp_restart();
    }

    int counter = 0;    
      while (true) {
          
        gpio_set_level(LED_PIN,1);
        vTaskDelay(pdMS_TO_TICKS(500));
        gpio_set_level(LED_PIN,0);
        vTaskDelay(pdMS_TO_TICKS(500));

        counter++;

        if (counter >= 30) {
            counter = 0;
            bool updated = ota_check_and_update();
            if (updated) {
                esp_restart();
            }
        }
          
      }
   
}

