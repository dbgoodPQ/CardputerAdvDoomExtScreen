/**
 * @file spi_display.hpp
 * @brief Display driver for an external Waveshare 2" ST7789V LCD (240x320, IPS)
 *        wired to the M5Stack Cardputer ADV header.
 *
 * Original built-in panel config by Forairaaaaa (2023). This version drives the
 * external panel only; the built-in 135x240 panel is left uninitialised and its
 * backlight is switched off (see builtin_lcd_off()).
 *
 * Wiring (Cardputer ADV -> Waveshare 2inch LCD module):
 *   DIN (MOSI) G14  shared with the SD card
 *   CLK (SCK)  G40  shared with the SD card
 *   CS         G5
 *   DC         G6
 *   RST        G3
 *   BL         G4   backlight, software-controlled
 *   VCC        3V3  set the module's voltage selector to 3.3V
 *   GND        GND
 *
 * Every pin and the SPI clock can be overridden with a compile definition
 * (see main/CMakeLists.txt, e.g. idf.py build -DEXT_SPI_SPEED=80000000).
 */
#pragma once

#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <driver/gpio.h>

// ---- External ST7789V panel -------------------------------------------------
#ifndef EXT_TFT_MOSI
#define EXT_TFT_MOSI 14
#endif
#ifndef EXT_TFT_SCLK
#define EXT_TFT_SCLK 40
#endif
#ifndef EXT_TFT_CS
#define EXT_TFT_CS   5
#endif
#ifndef EXT_TFT_DC
#define EXT_TFT_DC   6
#endif
#ifndef EXT_TFT_RST
#define EXT_TFT_RST  3
#endif
#ifndef EXT_TFT_BL
#define EXT_TFT_BL   4
#endif
#ifndef EXT_SPI_SPEED
#define EXT_SPI_SPEED 40000000  // 40 MHz, same default as LauncherDual. Try 80 MHz with short wires.
#endif

#ifndef EXT_TFT_ROTATION
#define EXT_TFT_ROTATION 1      // landscape; use 3 to flip it 180 degrees
#endif

#define EXT_TFT_WIDTH  320      // landscape
#define EXT_TFT_HEIGHT 240

// ---- Built-in Cardputer ADV panel (not used, backlight only) ----------------
#define BUILTIN_LCD_BL_PIN 38

class LGFX_ExtST7789 : public lgfx::LGFX_Device {
    lgfx::Panel_ST7789 _panel_instance;
    lgfx::Bus_SPI _bus_instance;
    lgfx::Light_PWM _light_instance;

   public:
    LGFX_ExtST7789(void) {
        {
            auto cfg = _bus_instance.config();

            cfg.spi_host   = SPI2_HOST;
            cfg.spi_mode   = 0;
            cfg.pin_mosi   = EXT_TFT_MOSI;
            cfg.pin_miso   = -1;            // write-only; SD MISO (G39) stays free
            cfg.pin_sclk   = EXT_TFT_SCLK;
            cfg.pin_dc     = EXT_TFT_DC;
            cfg.spi_3wire  = true;
            cfg.freq_write = EXT_SPI_SPEED;
            cfg.freq_read  = 16000000;

            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }
        {
            auto cfg = _panel_instance.config();

            cfg.pin_cs        = EXT_TFT_CS;
            cfg.pin_rst       = EXT_TFT_RST;
            cfg.pin_busy      = -1;
            cfg.memory_width  = 240;
            cfg.memory_height = 320;
            cfg.panel_width   = 240;
            cfg.panel_height  = 320;
            cfg.offset_x      = 0;
            cfg.offset_y      = 0;
            cfg.offset_rotation = 0;
            cfg.readable      = false;
            cfg.invert        = true;   // IPS panel: needs INVON
            cfg.rgb_order     = false;
            cfg.dlen_16bit    = false;
            cfg.bus_shared    = false;  // SCK/MOSI are the SD card's lines, but this firmware never mounts the SD card

            _panel_instance.config(cfg);
        }
        {
            auto cfg = _light_instance.config();

            cfg.pin_bl      = EXT_TFT_BL;
            cfg.invert      = false;
            cfg.freq        = 12000;
            cfg.pwm_channel = 7;

            _light_instance.config(cfg);
            _panel_instance.setLight(&_light_instance);
        }

        setPanel(&_panel_instance);
    }
};

// The built-in panel is never initialised, so turn its backlight off instead of
// leaving a lit, blank screen.
static inline void builtin_lcd_off(void) {
    gpio_reset_pin((gpio_num_t)BUILTIN_LCD_BL_PIN);
    gpio_set_direction((gpio_num_t)BUILTIN_LCD_BL_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)BUILTIN_LCD_BL_PIN, 0);
}
