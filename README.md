# 24h Circular Radio Clock (Funkuhr) for ESP-IDF

24-hour circular clock display component and demo application for round displays (GC9A01 240x240, LovyanGFX v1) on ESP32-P4, ESP32-S3, and ESP32.

![Target](https://img.shields.io/badge/Target-ESP32--P4%20%7C%20S3%20%7C%20Classic-blue)
![IDF](https://img.shields.io/badge/ESP--IDF-v6.0.2-green)
![Display](https://img.shields.io/badge/Display-GC9A01%20240x240-orange)

---

## Dial Design Specification (Dual-Ring Watchmaker Architecture)

```
                      [CY-36: Antenna ▲))) / "24" Badge]
                                      |
                      [CY-8: Digital HH : MM : SS]
                                      |
                           [CY+14: Date "Fr, 16. Okt"]
                                      |
                      [CY+45: Approximate Moon Phase]
                                      |
  [R 69..72: Blue 60m Comet] <--------+--------> [R 82: 60m Scale (60, 05..55)]
                                      |
  [R 96: Trennschiene (Ticks + Seconds + Pac-Man Dots + Radar)]
                                      |
  [R 108: 24h Scale (1..24)] <--------+--------> [R 116..119: Red 24h Comet]
```

### Detailed Geometry & Features:
- **R = 116..119**: **Red 24h-Hour Comet** (Fading tail 35°, glowing tip bead $r=3\text{px}$).
- **R = 108**: **24h Hour Scale** (10pt Noto Sans font, 24 at top, 12 at bottom; active hour pure white, inactive dark slate gray; airy spacing).
- **R = 96**: **Trennschiene Divider Track** with quarter tick lines ($R=92..100$), 5-minute ticks ($R=93.5..98.5$), and intermediate minute dots.
- **R = 96**: **Pac-Man Interactive Seconds Pip**:
  - The luminous amber pip orbits at 1 Hz, expanding to radiant pulses on 5-minute and quarter marks.
  - **Dot Consumption**: As the pip travels forward, it "eats" intermediate dots in its path, creating an analog visual progress ring for the current minute.
  - **Minute Regeneration Wave**: At second 0, a cascading luminous wave sweeps clockwise around the dial (0..650ms), respawning all dots with a soft neon bloom.
- **R = 82**: **60m Minute Scale** (8pt Noto Sans font, 60 at top, 05..55, 30 at bottom; active 5m interval pure white, inactive dark slate gray).
- **R = 69..72**: **Blue/Cyan 60m Minute Comet** (Fading tail 45°, glowing tip bead $r=3\text{px}$).
- **Compact Watchmaker Center ($R \le 48$)**:
  - **CY - 36**: Circular Badge with vector radio tower `▲)))` during search, `?` when time is unknown, `H` during holdover, or `24` / `AM` / `PM` in normal display mode.
  - **CY - 8**: Digital readout `HH : MM : SS` (16pt Noto Sans, 18px horizontal step: HH crimson red, colons slate gray, MM electric cyan, SS luminous amber).
  - **CY + 14**: Clean date display with DIN format (e.g. `Fr, 16. Okt` / `Fri 16 Oct`).
  - **CY + 45**: Approximate calendar-day Moon phase (Radius 10).

---

## Typography

Native pre-rendered Google Noto Sans U8g2 fonts (SIL Open Font License 1.1):
- **`noto_sans_8.h`** (`FONT_SMALL`): Date, 60m minute markers, AM/PM badges.
- **`noto_sans_10.h`** (`FONT_SCALE`): 24h hour numbers.
- **`noto_sans_16.h`** (`FONT_DIGITAL`): Digital time readout `HH : MM : SS`.

---

## Time Synchronization Status Display

The component is a display/rendering library. The application supplies time,
date, validity, and source status; the component does not acquire or validate
time. The demo uses SNTP where the target has a supported Wi-Fi backend.
Network synchronization does not guarantee atomic-clock accuracy.
Call the clock API from one application task; event callbacks should hand off
updates to that task rather than calling rendering or setters concurrently.

1. **Caller-provided status**:
   - `CIRCULAR_RADIO_UNKNOWN`: No valid time source has been established; the
     display shows `--:--:--`.
   - `CIRCULAR_RADIO_SEARCHING`: The application reports an active sync attempt.
   - `CIRCULAR_RADIO_LOCKED`: The application reports a confirmed sync event;
     this is not a measurement of ongoing reception or accuracy.
   - `CIRCULAR_RADIO_FREE_RUN`: The application reports local-clock holdover.
   - A confirmed synchronization cannot be reported as `LOCKED` while the
     supplied time is invalid. The demo keeps `SEARCHING` visible during initial
     acquisition and keeps holdover visible until a new sync succeeds after
     connectivity returns.

2. **Antenna Symbol `▲)))` (CY - 36)**:
   - In `SEARCHING`: Animated radio waves radiating dynamically ($0 \to 1 \to 2 \to 3$).
   - After a confirmed sync: A brief visual confirmation; it is not a precision
     or atomic-time measurement.
   - In Smart Auto mode, `?` marks unknown time and `H` marks local-clock
     holdover without an active synchronization source.

3. **Radar Sweep Animation (R = 96)**:
   - During `SEARCHING`, an electric phosphor radar sweep rotates across the Trennschiene, illuminating tick marks and dots in its wake.

4. **Synchronization transition animation**:
   - Upon a caller-reported sync event, both comet pointers execute a visual
     multi-turn sweep. This animation does not calibrate or adjust the clock.

5. **Time synchronization**:
   - The example currently implements Wi-Fi/SNTP only on targets with built-in
     Wi-Fi support. DCF77 and GPS/NMEA are not implemented.

The date-based moon icon is an approximate visualization using a mean
synodic-month model. It is not an ephemeris or a precision astronomical
calculation. Date and moon phase are hidden unless both time validity and
calendar-date validity are true. A custom date string is caller-formatted and
does not provide calendar data for moon-phase calculation. Custom strings are
limited to 47 bytes; longer strings are rejected and logged.

---

## Universal Target & Kconfig Configuration

The component is target-agnostic and supports **ESP32-P4**, **ESP32-S3**, **ESP32-C6**, and classic **ESP32**. All hardware pins and user preferences can be configured via `idf.py menuconfig`.

### Hardware Pinout & Defaults

| Signal | ESP32-P4 Default | ESP32-S3 Default | Classic ESP32 | Description |
|---|---|---|---|---|
| MOSI | **GPIO 28** | GPIO 11 | GPIO 23 | SPI Master Out |
| SCLK | **GPIO 27** | GPIO 12 | GPIO 18 | SPI Clock |
| CS   | **GPIO 24** | GPIO 10 | GPIO 5  | Chip Select |
| DC   | **GPIO 25** | GPIO 9  | GPIO 4  | Data / Command |
| RST  | **GPIO 26** | GPIO 14 | GPIO 2  | Reset (-1 if tied to EN) |
| BKL  | **-1** (VCC) | -1      | -1      | Backlight control |

### Menuconfig Settings

Run `idf.py menuconfig` to configure:
- **`Component config -> Circular 24h Clock Display Configuration`**:
  - SPI Host and Pin assignments
  - SPI Bus frequency (up to 80 MHz, default 40 MHz)
- **`Clock Demo Application Options`**:
  - **WiFi & NTP**: SSID, WPA2/WPA3 password, POSIX Timezone (`CET-1CEST,M3.5.0,M10.5.0/3` for automatic daylight saving)
  - **Language Selection**: German (`Fr, 16. Okt`) or English (`Fri, Oct 16`)
  - **Date Format**: DIN Abbreviated (`Fr, 16. Okt`), Full Weekday (`Freitag 16.10`), or Numeric (`16.10.2026`)
  - **Center Badge Mode**: Smart Auto (`▲)))` during search, clean `24` when locked), Always Antenna, or Always Mode (`24`/`AM`/`PM`)

---

## Build and Flash

```powershell
. C:\esp\v6.0.2\esp-idf\export.ps1
cd D:\github-repositorys\esp-idf-clock-circular-display\examples\clock_demo
idf.py set-target esp32p4   # or esp32s3, esp32
idf.py build
idf.py -p COM4 flash monitor
```
