#pragma once
#ifdef __cplusplus
extern "C" {
#endif

// Copy saves from the SD card into flash if flash has none. Call before the screen starts.
void sd_backup_import_at_boot(void);
// Copy one save slot from flash to the SD card. The SPI bus must be free.
void sd_backup_slot(int slot);

#ifdef __cplusplus
}
#endif
