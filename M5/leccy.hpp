/**
  M5 Libraries: 
  - [M5Unified](https://github.com/m5stack/M5Unified)
  * [M5GFX](https://github.com/m5stack/M5GFX)

  Arduino libraries:
  - WiFi.h (https://docs.arduino.cc/libraries/wifi/)
  * Networking
  * NetworkClientSecure

  External libraries:
  - HTTPClient.h (https://github.com/amcewen/HttpClient)
  - sntp.h 
  - ArduinoJSON.h (https://arduinojson.org/)
*/
#pragma once

#include <M5Unified.hpp> //change to hpp
#include <SPIFFS.h>
#include <ctime>
#include <WiFi.h>
#include <HTTPClient.h>
#include <sntp.h>
#include <ArduinoJson.h>
#include <ArduinoJson.hpp>

//Struct for prices
typedef struct s_prices
{
    float priceNow = 0;
    float pricePlus15 = 0;
    float pricePlus30 = 0;
} t_prices;

t_prices priceList;

//Error flags
bool errorClock = false;
bool errorJSON = false;
bool errorFile = false;

/**
    System settings
*/
#define TEXT_SIZE   1.8
#define TICK_DELAY  500
#define SLEEP_OFFSET 60
#define SLOT_GRACE 60   // seconds; tolerates waking up to a minute early

/**
    WiFi credentials, very secure, so not here.
*/
#define WIFI_TIMEOUT    300

/**
    NTP timezone and servers.
*/
#define NTP_TIMEZONE  "Europe/Helsinki"
#define NTP_SERVER1   "time1.mikes.fi"
#define NTP_SERVER2   "fi.pool.ntp.org"
#define NTP_SERVER3   "utcnist.colorado.edu"
#define NTP_UPDATE_RATE 96

/**
    Filesystem settings
*/
#define FILE_JSON_NAME "/latest-prices.json"
#define FILE_TEXT_NAME "/allprices.txt"
#define FILE_TEMP_NAME "/temp.txt"
#define FILE_COUNT_NAME "/updatecount.txt"

/**
    Price API details.
*/
#define PRICE_INTERVAL 15
#define EPOCH_OFFSET 1800
#define PRICE_UPDATE_TIME 16
#define PRICE_LIST_ADD "https://api.porssisahko.net/v2/latest-prices.json"
#define PRICE_DATE_MASK "%04d-%02d-%02dT%02d:%02d:00.000Z"
#define PRICE_DATE_MASK2 "%FT%H:"
