/*
 * SD card backup of save games.
 *
 * Save games live in internal flash (/sd/doom/DOOMn.dsg). The SD card only holds
 * a copy in /doom/ on the card, so progress survives reinstalling the firmware:
 *   - at boot, before the screen starts: if flash has no saves, import them
 *     from the card;
 *   - after each save: copy that slot to the card (main.cpp releases the screen's
 *     SPI bus first, so the card and the screen never use the bus at once).
 * Any SD failure is logged and ignored: the save in flash is always kept.
 */
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "esp_vfs_fat.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "sdmmc_cmd.h"

#include "sd_backup.h"

#define SD_PIN_MOSI 14
#define SD_PIN_MISO 39
#define SD_PIN_SCLK 40
#define SD_PIN_CS   12
#define SD_HOST     SPI2_HOST

#define SD_MOUNT    "/sdc"
#define SD_DIR      SD_MOUNT "/doom"
#define FLASH_DIR   "/sd/doom"
#define SAVE_SLOTS  8

static sdmmc_card_t *s_card;

static bool sd_mount(void)
{
    spi_bus_config_t bus = {
        .mosi_io_num = SD_PIN_MOSI,
        .miso_io_num = SD_PIN_MISO,
        .sclk_io_num = SD_PIN_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
    };
    esp_err_t err = spi_bus_initialize(SD_HOST, &bus, SPI_DMA_CH_AUTO);
    if (err != ESP_OK) {
        printf("[sd] bus init failed: %s\n", esp_err_to_name(err));
        return false;
    }

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SD_HOST;

    sdspi_device_config_t slot = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot.gpio_cs = SD_PIN_CS;
    slot.host_id = SD_HOST;

    esp_vfs_fat_sdmmc_mount_config_t mcfg = {
        .format_if_mount_failed = false,   // never wipe the user's card
        .max_files = 2,
        .allocation_unit_size = 16 * 1024,
    };
    err = esp_vfs_fat_sdspi_mount(SD_MOUNT, &host, &slot, &mcfg, &s_card);
    if (err != ESP_OK) {
        printf("[sd] no card or mount failed: %s\n", esp_err_to_name(err));
        spi_bus_free(SD_HOST);
        s_card = NULL;
        return false;
    }
    return true;
}

static void sd_unmount(void)
{
    if (s_card) {
        esp_vfs_fat_sdcard_unmount(SD_MOUNT, s_card);
        s_card = NULL;
    }
    spi_bus_free(SD_HOST);
}

static bool copy_file(const char *src, const char *dst)
{
    static char buf[512];
    FILE *in = fopen(src, "rb");
    if (!in) return false;
    FILE *out = fopen(dst, "wb");
    if (!out) { fclose(in); return false; }

    bool ok = true;
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0) {
        if (fwrite(buf, 1, n, out) != n) { ok = false; break; }
    }
    fclose(in);
    if (fclose(out) != 0) ok = false;
    return ok;
}

static bool file_exists(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0;
}

void sd_backup_import_at_boot(void)
{
    char src[48], dst[48];

    for (int i = 0; i < SAVE_SLOTS; i++) {
        snprintf(dst, sizeof(dst), FLASH_DIR "/DOOM%d.dsg", i);
        if (file_exists(dst)) {
            printf("[sd] flash already has saves, no import\n");
            return;
        }
    }

    if (!sd_mount()) return;

    int copied = 0;
    for (int i = 0; i < SAVE_SLOTS; i++) {
        snprintf(src, sizeof(src), SD_DIR "/DOOM%d.dsg", i);
        snprintf(dst, sizeof(dst), FLASH_DIR "/DOOM%d.dsg", i);
        if (file_exists(src) && copy_file(src, dst)) copied++;
    }
    printf("[sd] imported %d save(s) from the card\n", copied);

    sd_unmount();
}

void sd_backup_slot(int slot)
{
    char src[48], dst[48];

    if (slot < 0 || slot >= SAVE_SLOTS) return;
    if (!sd_mount()) return;

    mkdir(SD_DIR, 0775);
    snprintf(src, sizeof(src), FLASH_DIR "/DOOM%d.dsg", slot);
    snprintf(dst, sizeof(dst), SD_DIR "/DOOM%d.dsg", slot);
    printf("[sd] backup slot %d: %s\n", slot, copy_file(src, dst) ? "ok" : "FAILED");

    sd_unmount();
}
