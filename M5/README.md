# Leccy

A low-power electricity price display for the M5Stack Core Ink (ESP32, e-paper).
It shows the price for the current and following two 15-minute periods, refreshed every 15 minutes.

Prices come from the [porssisahko.net](https://porssisahko.net/api) API, which aggregates NordPool day-ahead data.

## Display

``` text
Bat: <battery %> - <error codes> <update time>
              <price now>
              <price in 15>
              <price in 30>
```

Prices are shown exactly as the API supplies them (c/kWh).
A price shown as `99999` means that price could not be read.

### Error codes

Shown on the battery level line.

| Flag | Meaning |
|------|---------|
| `V`  | RTC voltage fell below minimum, clock may be unreliable |
| `C`  | NTP failed, or the system clock is unset |
| `J`  | JSON price file download failed |
| `F`  | Local price file missing, unreadable, or current slot was not found in it |

`BAT LOW` appears when the battery is below 10%.

## How it works

```
 wake up (RTC alarm or deep-sleep timer)
   |
   v
 initialise serial system, M5 config, display, SPIFFS file system
   |
   v
 prices file missing? --yes--> download JSON, write file
   |
   v
 calculate price slot (UTC), read 3 prices from file
   |
   v
 slot missing, or <= ~10 h of data left? --yes--> download JSON, rewrite file
   |
   v
 draw display (prices, battery, flags, time)
   |
   v
 set RTC alarm for next quarter, on bat: power off / on usb: deep sleep
```

1. **Wake.** Wake roughly every 15 minutes. On battery the RTC alarm powers it on after `powerOff()`. On USB power `powerOff()` doesn't work, so a deep-sleep timer times the wake up.
2. **Time.** The system clock is taken from the RTC. The RTC is set from NTP on cold start and whenever the price file is refreshed.
3. **Lookup.** The target slot is 30 minutes ahead of now, rounded down to a 15-minute boundary in UTC.
4. **Price file.** `/allprices.txt` in SPIFFS holds one `startDate,price` line per 15-minute period, newest first. The first matching line is therefore the price in 30 minutes, and the next two lines are the price in 15 minutes and the current price.
5. **Refresh.** The API publishes new prices after 14:00 each day. The file is re-downloaded when the current slot is not found, or when 40 lines or fewer sit above the current slot (about 10 hours of future data). The new file is written to `/temp.txt` then renamed over the old one.

## Hardware

- [M5Stack Core Ink](https://docs.m5stack.com/en/core/CoreInk): ESP32, e-paper display, RTC (BM8563), internal flash, WiFi.

## Software requirements

- Arduino IDE (or similar) with the ESP32, and M5Stack Core Ink selected.
- Libraries:
  - [M5Unified](https://github.com/m5stack/M5Unified)
  - [M5GFX](https://github.com/m5stack/M5GFX)
  - [ArduinoJson](https://arduinojson.org/) (written against the v6 API)
  - ESP32 Arduino core libraries: WiFi, HTTPClient, SPIFFS

## Files

| File | Purpose |
|------|---------|
| `leccy.hpp` | Includes, settings (`#define`s), global price struct and error flags |
| `leccy.ino` | Required entry point; just an empty `loop()` because the device sleeps at the end of `setup()` |
| `leccy_setup.ino` | `setup()`: the wake cycle |
| `leccy_helpers.ino` | WiFi, NTP, JSON download, file handling, price lookup, display and sleep |

## Configuration

| Define | Purpose |
|--------|---------|
| `WIFI_SSID`, `WIFI_PASSWORD` | WiFi credentials. |
| `PRICE_LIST_ADD` | API endpoint |

## Known limitations

- The battery line clock adds a fixed 2 hours to the RTC (UTC) time. This is a quick kludge for indication purposes.
- If the network or API is down, each wake spends up to the WiFi timeout retrying while stale data is displayed. This will eventually wear the battery down.
- Sleeping on USB power uses a different wake mechanism from battery, so wake timing can differ slightly between the two.
- Tracking of when to download the new JSON is messy.
