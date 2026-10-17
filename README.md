# 24-Hour Circular Clock Display for ESP-IDF

[![Component Registry](https://img.shields.io/badge/ESP--IDF-v5.0%20--%20v6.0+-blue.svg)](https://components.espressif.com/)
[![Target](https://img.shields.io/badge/Target-ESP32--P4%20%7C%20S3%20%7C%20C6%20%7C%20Classic-orange.svg)]()
[![Display](https://img.shields.io/badge/Display-GC9A01%20240x240%20SPI-purple.svg)]()
[![Rendering](https://img.shields.io/badge/Engine-LovyanGFX%201.2.33-brightgreen.svg)]()
[![License](https://img.shields.io/badge/License-MIT-green.svg)]()

A high-precision, state-of-the-art **24-hour luxury circular clock display component** designed for round 240x240 GC9A01 SPI displays running on ESP-IDF (ESP32-P4, ESP32-S3, ESP32-C6, and ESP32).

Renders fluid dual-ring chapter scales, interactive animated comets, an orbital seconds pip with minute wave regeneration, a compact Swiss horology center cluster (with real-time approximate moon phase), full 10-language localization, and persistent NVS runtime configuration.

---

## Key Features

- **Luxury Dual-Ring Horology Architecture**:
  - Outer 24h crimson red comet pointer and airy 24h numerals.
  - Inner 60m electric cyan comet pointer and 60m minute track.
  - Concentric *Trennschiene* divider ring with interactive second pip, minute progress dot consumption, and cascading minute regeneration waves.
- **Center Cluster Watch Face**:
  - Top multi-mode badge (Smart Auto antenna $\blacktriangle$))), "24", "AM"/"PM", holdover "H", or unknown "?").
  - Digital readout `HH : MM : SS` rendered in crisp Google Noto Sans fonts.
  - Formatted calendar date in 10 European languages.
  - Real-time calendar-day astronomical Moon Phase (earthshine shadow & silver illumination).
- **Comprehensive Multilingual Localization (10 BCP 47 Locales)**:
  - Built-in Latin-1 font coverage for German, British English, American English, French, Spanish, Portuguese, Italian, Danish, Norwegian, and Swedish.
  - Dynamic discovery API for GUI menus and serial consoles.
- **Persistent Settings Manager (`clock_settings.h`)**:
  - Dedicated NVS partition storage for locale, POSIX timezone, date format, badge mode, 12h/24h mode, and backlight brightness.
  - Live runtime updating: changes take immediate effect on screen; saving to flash is explicitly controllable to prevent flash wear.
- **Dual Operating Modes**:
  - **Standalone Mode**: Component directly drives GC9A01 panel and SPI bus with auto-managed backlight PWM.
  - **Embedded / Headless Mode**: Supply `NULL` config to `circular_clock_init(NULL)` and render into an external LovyanGFX sprite buffer (`circular_clock_render_to_sprite`) — ideal for idle screen integration in larger UI frameworks and audio/voice applications (e.g., TTS).

---

## Dial Design Specification

```
                      [CY-36: Antenna ▲))) / "24" Badge]
                                      |
                      [CY-8: Digital HH : MM : SS]
                                      |
                           [CY+14: Date "Fr, 16. Okt"]
                                      |
                      [CY+45: Approximate Moon Phase]
                                      |
  [R 69..72: Cyan 60m Comet] <--------+--------> [R 82: 60m Scale (60, 05..55)]
                                      |
  [R 96: Trennschiene (Ticks + Seconds Pip + Pac-Man Dots + Radar)]
                                      |
  [R 108: 24h Scale (1..24)] <--------+--------> [R 116..119: Red 24h Comet]
```

### Detailed Geometry & Dimensions ($240 \times 240$, Center $CX=120, CY=120$)

| Radius / Offset | Element | Description |
|---|---|---|
| **$R = 116..119$** | **24h Hour Comet** | Fading crimson tail ($35^\circ$), glowing tip bead ($r=3\text{px}$). |
| **$R = 108$** | **24h Hour Scale** | 10pt Noto Sans font; active hour pure white, inactive dark slate gray. |
| **$R = 96$** | **Trennschiene Divider** | Quarter tick marks ($R=92..100$), 5m ticks ($R=93.5..98.5$), intermediate dots. |
| **$R = 96$** | **Interactive Second Pip** | Warm amber pip orbits at 1 Hz; consumes intermediate dots in its path (analog progress); triggers a cascading wave respawn at second 0. |
| **$R = 82$** | **60m Minute Scale** | 8pt Noto Sans font (`60`, `05`..`55`, `30`). |
| **$R = 69..72$** | **60m Minute Comet** | Fading electric cyan tail ($45^\circ$), glowing tip bead ($r=3\text{px}$). |
| **$CY - 36$** | **Top Circular Badge** | Displays vector antenna symbol during sync/search, or clean mode ("24", "AM", "PM"). |
| **$CY - 8$** | **Digital Time** | 16pt Noto Sans `HH : MM : SS` (18px horizontal step; color-coded matching comet tracks). |
| **$CY + 14$** | **Date Readout** | Localized date formatted according to selected BCP 47 locale and format preference. |
| **$CY + 45$** | **Astronomical Moon** | Earthshine circle ($r=10\text{px}$) with phase illumination computed via synodic month approximation. |

---

## Supported Locales (10 Languages)

All locale names and abbreviations use characters within the bundled Latin-1 font subset:

| Index | BCP 47 Tag | Language | Short / Full Example | Separator Style |
|:---:|:---:|:---|:---|:---:|
| **0** | `de-DE` | Deutsch (German) | `Fr, 16. Okt` / `Freitag, 16. Okt` | Day.Month (`16.10`) |
| **1** | `en-GB` | English (United Kingdom) | `Fri, 16 Oct` / `Friday, 16 Oct` | Day Month (`16.10`) |
| **2** | `en-US` | English (United States) | `Fri, Oct 16` / `Friday, Oct 16` | Month/Day (`10/16`) |
| **3** | `fr-FR` | Français (French) | `ven. 16 oct.` / `vendredi 16 oct.` | Day/Month (`16/10`) |
| **4** | `es-ES` | Español (Spanish) | `vie., 16 oct.` / `viernes, 16 oct.` | Day/Month (`16/10`) |
| **5** | `pt-PT` | Português (Portuguese) | `sex., 16 de out.` / `sexta-feira, 16 de out.` | Day/Month (`16/10`) |
| **6** | `it-IT` | Italiano (Italian) | `ven 16 ott` / `venerdì 16 ott` | Day/Month (`16/10`) |
| **7** | `da-DK` | Dansk (Danish) | `fre. 16. okt.` / `fredag 16. okt.` | Day.Month (`16/10`) |
| **8** | `nb-NO` | Norsk Bokmål (Norwegian) | `fre. 16. okt.` / `fredag 16. okt.` | Day.Month (`16/10`) |
| **9** | `sv-SE` | Svenska (Swedish) | `fre 16 okt.` / `fredag 16 okt.` | Day/Month (`16/10`) |

*Bare 2-letter tags (`de`, `en`, `fr`, `es`, `pt`, `it`, `da`, `nb`, `sv`) are accepted as case-insensitive aliases.*

---

## Hardware Pinout & Target Defaults

All pins are fully configurable via `idf.py menuconfig` under `Component config -> Circular 24h Clock Display Component`:

| Signal | ESP32-P4 Default | ESP32-S3 Default | Classic ESP32 | Description |
|---|---|---|---|---|
| **MOSI** | **GPIO 28** | GPIO 11 | GPIO 23 | SPI MOSI / SDA |
| **SCLK** | **GPIO 27** | GPIO 12 | GPIO 18 | SPI Clock |
| **CS**   | **GPIO 24** | GPIO 10 | GPIO 5  | SPI Chip Select |
| **DC**   | **GPIO 25** | GPIO 9  | GPIO 2  | Data / Command |
| **RST**  | **GPIO 26** | GPIO 14 | GPIO 4  | Reset (-1 if unmanaged) |
| **BKL**  | **-1** (VCC) | -1      | -1      | Backlight PWM (-1 if unmanaged) |

SPI clock frequency defaults to 40 MHz (supports up to 80 MHz depending on trace length).

---

## Public API Reference

The component exports two headers:
- `circular_clock.h`: Clock rendering, time/date feeding, layout preferences, and display lifecycle.
- `clock_settings.h`: Persistent NVS settings manager for runtime configuration.

### 1. Initialization & Lifecycle

```c
#include "circular_clock.h"

// Standalone mode: initializes SPI bus, GC9A01 panel, sprite, and settings manager
circular_clock_config_t cfg = {
    .spi_host     = SPI2_HOST,
    .pin_mosi     = 28,
    .pin_sclk     = 27,
    .pin_cs       = 24,
    .pin_dc       = 25,
    .pin_rst      = 26,
    .pin_bkl      = -1,
    .spi_clock_hz = 40000000
};
ESP_ERROR_CHECK(circular_clock_init(&cfg));

// Headless / sprite-only embedded mode (no display panel hardware initialized):
ESP_ERROR_CHECK(circular_clock_init(NULL));
```

### 2. Time & Date Input

```c
// Feed current time (typically in a 30 FPS rendering task)
circular_clock_set_time(hour, minute, second, millisecond);
circular_clock_set_time_valid(true);

// Feed calendar date (triggers moon phase computation)
circular_clock_set_date(2026, 10, 17, 6 /* Saturday */);

// Optional: override with custom date string (up to 47 chars)
circular_clock_set_date_str("Sat, 17. Oct");
```

### 3. Rendering

```c
// Standalone: renders to internal sprite and pushes to physical GC9A01 LCD
circular_clock_render();

// Embedded mode: render into an external LovyanGFX sprite buffer (240x240)
circular_clock_render_to_sprite(my_sprite_ptr);
```

### 4. Runtime Settings & NVS Persistence (`clock_settings.h`)

All settings changes via `clock_settings_set_*` immediately update the live clock face. Changes are committed to flash when calling `clock_settings_save(NULL)`:

```c
#include "clock_settings.h"

// Initialize settings from NVS (or write defaults on first boot)
clock_settings_init();

// Change language at runtime (takes effect immediately on display)
clock_settings_set_locale("en-US");

// Change timezone (applies POSIX TZ and runs tzset())
clock_settings_set_timezone("CET-1CEST,M3.5.0,M10.5.0/3");

// Change date format style
clock_settings_set_date_format(CLOCK_DATE_FMT_ABBREV); // NUMERIC, ABBREV, FULL_WEEKDAY

// Change top badge mode
clock_settings_set_badge_mode(CLOCK_BADGE_SMART_AUTO); // SMART_AUTO, ALWAYS_ANTENNA, ALWAYS_MODE

// Change 12h/24h mode
clock_settings_set_ampm_mode(false); // false = 24h, true = 12h AM/PM

// Change brightness (0..100 %)
clock_settings_set_brightness(80);

// Commit runtime settings to NVS flash
clock_settings_save(NULL);

// Reset to factory defaults
clock_settings_reset_to_defaults();
```

### 5. Dynamic Locale Discovery

```c
uint8_t count = circular_clock_get_locale_count(); // Returns 10
for (uint8_t i = 0; i < count; i++) {
    char tag[16];
    circular_clock_get_locale_tag(i, tag, sizeof(tag));
    printf("Locale [%u]: %s\n", i, tag);
}
```

---

## Example Project (`examples/clock_demo`)

A complete production-ready example demonstrating all features is included in `examples/clock_demo`.

### Features Demonstrated:
- GC9A01 SPI display initialization on ESP32-P4.
- Wi-Fi connection and automatic SNTP internet time synchronization (`pool.ntp.org`, `time.google.com`, `ptbtime1.ptb.de`).
- Automatic Daylight Saving Time (DST) handling using POSIX timezone rules.
- 30 FPS rendering loop with animated comets and orbital seconds pip.
- Optional auto-cycling language demonstration (`CONFIG_CLOCK_LOCALE_DEMO`).
- Persistent NVS settings verification.

### Build and Flash (ESP32-P4 on COM4):

```powershell
# In PowerShell:
$env:IDF_PATH="C:\esp\v6.0.2\esp-idf"
$env:ESP_IDF_VERSION="6.0.2"
$env:IDF_PYTHON_ENV_PATH="C:\Espressif\tools\python\v6.0.2\venv"
$env:PATH="C:\Espressif\tools\ninja\1.12.1;C:\Espressif\tools\idf-exe\1.0.3;C:\Espressif\tools\ccache\4.8.3\ccache-4.8.3-windows-x86_64;C:\Espressif\tools\riscv32-esp-elf\esp-15.2.0_20251204\riscv32-esp-elf\bin;" + $env:PATH

cd D:\github-repositorys\esp-idf-clock-circular-display\examples\clock_demo
python.exe C:\esp\v6.0.2\esp-idf\tools\idf.py build
python.exe C:\esp\v6.0.2\esp-idf\tools\idf.py -p COM4 flash monitor
```

---

## License

This component is distributed under the **MIT License**.
Fonts bundled under `include/fonts/` are sourced from Google Noto Sans and licensed under the **SIL Open Font License 1.1**.
