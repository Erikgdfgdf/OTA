#ifndef OTA_H
#define OTA_H

void mark_firmware_valid();
bool ota_check_and_update();
void ota_task(void* param);

#endif // OTA_H