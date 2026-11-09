# Cardputer ADV Doom — External Screen

Doom for the **M5Stack Cardputer ADV**, rendered on an external **Waveshare 2" LCD (ST7789V, 240×320 IPS)** instead of the built-in 135×240 screen.

Everything else (keyboard, sound, music, other controls, embedded WAD, 6 MB partition table) is unchanged from the original project — see [Credits](#credits).

---

## What changed

| Area | Original | This fork |
|------|----------|-----------|
| Display | Built-in ST7789, 135×240 | External Waveshare 2" ST7789V, 240×320 IPS (used as 320×240 landscape) |
| Picture | 240×160 frame scaled down to 202×135 | 240×160 frame scaled ×4/3 to **320×213**, centred (aspect kept, thin black bars top/bottom) |
| Built-in screen | Used | Not initialised; its backlight (G38) is switched off |
| Backlight | G38 (built-in) | **G4**, PWM-controlled |
| SPI | SPI2 on G35/G36 at 80 MHz | SPI2 on **G14/G40** (the SD card's lines) at **40 MHz** |
| Turn keys | Left `l`, right `'` | Left **`,`**, right **`/`** (also in menus and the map) |
| Menus | Same scaling as gameplay | Smoothed (anti-aliased) scaling while a menu is open, so text looks even |
| Settings | Not saved (GBA cartridge SRAM only) | Saved in flash (NVS): brightness, volumes, always run, messages |
| Brightness | Starts at 0 | Starts at **4** (of 0–5) until you change it |
| Save / load game | Needs an SD card that is never mounted, so it does nothing (load can crash) | Saved in a 1 MB FAT partition in internal flash; each save is also copied to the SD card (see below) |
| Build | Manual `idf.py build` | GitHub Action builds the Cardputer ADV firmware and uploads it as an artifact |

Code changes are limited to `components/GBADoom/source/g_game.c` (turn keys, brightness), `components/GBADoom/source/i_system_esps3.c` and `include/gba_functions.h` (settings in NVS), `partitions.csv` and `sdkconfig` (save storage), `main/sd_backup.c` (SD copy), `main/spi_display.hpp` (new panel driver), `main/main.cpp` (init + scaling), `main/CMakeLists.txt` (build-time overrides), `merge_firmware.sh` (bootloader offset fixed to `0x0` for ESP32-S3) and `.github/workflows/build-cardputer-adv.yml`.

---

## Wiring (Cardputer ADV header → Waveshare 2inch LCD module)

| Waveshare pin | Cardputer ADV | Notes |
|---------------|---------------|-------|
| DIN (MOSI) | G14 | shared with the SD card |
| CLK (SCK)  | G40 | shared with the SD card |
| CS  | G5 | |
| DC  | G6 | |
| RST | G3 | |
| BL  | G4 | backlight, software-controlled |
| VCC | 3V3 | set the module's voltage selector to **3.3V** |
| GND | GND | |

Same wiring as [LauncherDual](https://github.com/dbgoodPQ/LauncherDual), so one cable works for both. The SD card is only used for save backups, and only while the screen is paused (see below), so sharing G14/G40 causes no conflict. Leave G39 (SD MISO) and G12 (SD CS) unconnected to the screen.

---

## Save games and the SD card

- **Save / load** always use internal flash (`/sd/doom/DOOMn.dsg`), so loading never touches the card.
- **After each save**, the slot is copied to `/doom/DOOMn.dsg` on the SD card. The screen pauses for a moment and hands its SPI bus to the card for the copy.
- **At boot**, if flash has no saves (for example after reinstalling), they are copied back from the card.
- No card, or a card error, is logged and ignored: the save in flash is kept. The card must be FAT32; it is never formatted.

Reinstalling this firmware with the merged file keeps the saves in flash anyway (the save partition at `0x610000` is not overwritten). The SD copy matters after a full erase or after flashing a firmware with a different layout.

---

## Download

Every push to `main` publishes a [release](https://github.com/dbgoodPQ/CardputerAdvDoomExtScreen/releases/latest). Direct link to the latest merged image:

https://github.com/dbgoodPQ/CardputerAdvDoomExtScreen/releases/latest/download/cardputer-adv-doom-extscreen-merged.bin

From PowerShell (no browser, no login):

```powershell
Invoke-WebRequest -Uri "https://github.com/dbgoodPQ/CardputerAdvDoomExtScreen/releases/latest/download/cardputer-adv-doom-extscreen-merged.bin" -OutFile "cardputer-adv-doom-extscreen-merged.bin"
```

The same files are also on each Actions run as the artifact `cardputer-adv-doom-extscreen`.

| File | Flash address |
|------|---------------|
| `cardputer-adv-doom-extscreen-merged.bin` | `0x0` (everything in one file) |
| `bootloader.bin` | `0x0` |
| `partition-table.bin` | `0x8000` |
| `cardputer_doom.bin` | `0x10000` |

## Flash

Download mode: power **OFF**, hold **G0**, power **ON**, release G0.

```bash
esptool.py --chip esp32s3 --port /dev/ttyACM0 --baud 921600 write_flash 0x0 cardputer-adv-doom-extscreen-merged.bin
```

Flashing only the app is not enough on a fresh device: the partition table (6 MB app + 1 MB save storage) must be written too. The merged file includes it.

---

## Build options

Defaults are in `main/spi_display.hpp`. Override at build time:

```bash
idf.py -DEXT_SPI_SPEED=80000000 -DEXT_TFT_ROTATION=3 build
```

| Option | Default | |
|--------|---------|---|
| `EXT_SPI_SPEED` | `40000000` | Raise to `80000000` for more FPS if your wires are short; lower to `20000000` if the picture is noisy |
| `EXT_TFT_ROTATION` | `1` | `3` flips the picture 180° |
| `DOOM_SMOOTH_SCALE` | `1` | `0` never smooth (fastest), `1` smooth in menus, `2` always smooth (sharper-looking text, lower FPS) |
| `EXT_TFT_CS` / `DC` / `RST` / `BL` / `MOSI` / `SCLK` | `5` / `6` / `3` / `4` / `14` / `40` | Pin numbers |

From GitHub, **Actions → Build Cardputer ADV → Run workflow** takes the SPI speed and rotation as inputs.

Requires ESP-IDF **v5.5.2** (matches the committed `sdkconfig`).

---

## Troubleshooting

| Symptom | Try |
|---------|-----|
| Screen stays white or black | Check CS/DC/RST wiring and the 3.3V selector |
| Picture upside down | Build with `EXT_TFT_ROTATION=3` |
| Garbled picture / noise | Build with `EXT_SPI_SPEED=20000000` |
| Colours inverted | The driver sends INVON for an IPS panel; set `cfg.invert = false` in `spi_display.hpp` for a non-IPS module |

---

## Credits

- Original Cardputer ADV port: [zspuspoki/CardPuterAdvancedDoom](https://github.com/zspuspoki/CardPuterAdvancedDoom) (MIT) — the source this repository is forked from.
- Working ADV binary and flashing notes: [MAXXTANG/CardPuter-ADV-Doom](https://github.com/MAXXTANG/CardPuter-ADV-Doom).
- Original Cardputer Doom: [romalik/m5cardputer_doom](https://github.com/romalik/m5cardputer_doom), based on [doomhack/GBADoom](https://github.com/doomhack/GBADoom).
- Display library: [LovyanGFX](https://github.com/lovyan03/LovyanGFX).
- External screen wiring from [dbgoodPQ/LauncherDual](https://github.com/dbgoodPQ/LauncherDual).

MIT License — see [LICENSE](LICENSE).
