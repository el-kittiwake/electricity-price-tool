/**
*******************************************************************************
*   Program to fetch the current and future electricity prices from a NordPool
*   aggregating API.
*   Current API: https://porssisahko.net/api
*   Font: Orbitron_Light_24 or FreeSans18pt7b work with very similar scaling.
*         FreeSans needs the scale dropping by 0.1 - 0.2ish.
*         Orbitron looks neater, but FreeSans is easier to read.
*******************************************************************************
*/
#include "leccy.hpp"

/**
 * @brief Startup serial communication.
 * @note Serial output can be commented out once things are reliable, well tested and finished :D
 */
void serialInit(void)
{
    Serial.begin(115200);
    delay(TICK_DELAY * 2);

    Serial.println("\r\nSerial begin @ 115200");
}

/**
 * @brief Initialise M5 system.
*/
void deviceInit(void)
{
    auto cfg = M5.config();
    cfg.external_rtc  = true;
    cfg.clear_display = false;
    M5.begin(cfg);
    M5.Display.setFont(&FreeSans18pt7b);

    Serial.println("\r\nDevice initialised");
}

/**
 * @brief Set up and connect to WiFi.
 * @note Remember to disconnect once done.
 * @details Start WiFi with given credentials. Tick until set up.
 * If no connection is made after 5 minutes return false.
 */
bool wifiInit(void)
{
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    int tickCount = 0;
    while (WiFi.status() != WL_CONNECTED)
    {
        if (tickCount > WIFI_TIMEOUT)
            return (false);

        //Serial.print('.');
        delay(TICK_DELAY);
        ++tickCount;
    }
    //Serial.println(tickCount);

    Serial.println("\r\nWiFi initialised");

    return (true);
}

/**
 * @brief Sync NTP and RTC
 * @details Attempt NTP connection.
 *          If connection is not made after 3 minutes, returns false.
 *          Set time once synced.
*/
bool clockInit(void)
{
    if (!M5.Rtc.getIRQstatus())
    {
        //Serial.println("Beginning NTP sync....");
        /*if (!wifiInit())
        {
            Serial.println("WiFi connection failed. No NTP sync.");
            char buf[11] = {0};
            sprintf(buf, "WiFi fail");
            M5.Display.drawCenterString(buf, 100, 20);
            //sprintf(buf, "Restart");
            sprintf(buf, "No NTP");
            M5.Display.drawCenterString(buf, 100, 80);

            return (false);
        }*/

        //Serial.println("\r\n Syncing NTP....");
        int tickCount = 0;
        configTzTime(NTP_TIMEZONE, NTP_SERVER1, NTP_SERVER2, NTP_SERVER3);
        while (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED)
        {
            if (tickCount > WIFI_TIMEOUT)
            {
                //Serial.println("NTP not synced!");
                return (false);
            }

            //Serial.print(',');
            delay(TICK_DELAY);
            ++tickCount;
        }
        //Serial.println(tickCount);
        //Serial.println("\r\n NTP Synced.");

        //WiFi.disconnect();
        //Serial.println("\r\n WiFi disconnected.");

        time_t t = time(nullptr) + 1; // Advance one second.
        while (t > time(nullptr)) ;  /// Synchronization in seconds

        M5.Rtc.setDateTime(gmtime(&t));

        Serial.println("\r\nClock initialised");
    }

    return (true);
}

/**
 * @brief Attempts to fetch the latest_prices.json from the server and write them to file.
 * @details
    https://developer.mozilla.org/en-US/docs/Web/HTTP/Reference/Status
*/
bool getJson(void)
{
    bool ret = true;
    //Serial.println("Beginning WiFi connection for JSON get....");
    if (!wifiInit())
    {
        //Serial.print("WiFi connection failed. No JSON got.");
        return (false);
    }

    //Attempt to sync RTC with NTP
    if (!clockInit())
        errorClock = true;
    
    //SET up HTTP client, give address, check return code is 200 (OK, successful request),
    //If successful, attempt to stream data ArduinoJson deserialiser.
    //Serial.println("Start HTTP process....");
    HTTPClient http;

    //Send request
    http.useHTTP10(true);
    http.begin(PRICE_LIST_ADD);
    short httpCode = http.GET();
    if (httpCode == 200)
    {
        //Generated: https://arduinojson.org/v6/assistant/
        WiFiClient &stream = http.getStream();
        DynamicJsonDocument doc(32768); //ArduinoJson assistant recommended size
        DeserializationError error = deserializeJson(doc, stream);

        if (error)
        {
            //Serial.print("deserializeJson() failed: ");
            //Serial.println(error.c_str());
            ret = false;
        }
        else
            ret = processJSON(doc);
    }
    else
    {
        //Serial.print("http error: ");
        //Serial.println(httpCode);
        ret = false;
    }
    //Serial.print("HTTP process done.");
    http.end();

    //Disconnect WiFi and return retval.
    WiFi.disconnect();
    //Serial.println("WiFi disconnected. getJson() function complete.");
    return (ret);
}

/**
 * @brief Transfer startdate and price from JSON to csv file.
 * @details Helper for getJson()
*/
bool processJSON(DynamicJsonDocument &doc)
{
    File tempFile = SPIFFS.open(FILE_TEMP_NAME, FILE_WRITE);
    if (!tempFile)
        return (false);
    else
    {
        for (JsonObject price : doc["prices"].as<JsonArray>())
        {
            const char* startDate = price["startDate"]; // String: "2025-10-15T21:45:00.000Z"
            float price_price = price["price"];         // Float: 0.158
            char buf[37] = {0};
            snprintf(buf, sizeof(buf), "%s,%.3f", startDate, price_price);
            tempFile.println(buf);
        }
        tempFile.close();
        if (SPIFFS.exists(FILE_TEXT_NAME))
            SPIFFS.remove(FILE_TEXT_NAME);
        SPIFFS.rename(FILE_TEMP_NAME, FILE_TEXT_NAME);
    }

    return (true);
}

/**
 * @brief Wait for new minute if current minute is 1 less than quarter.
*/
void waitForQuarter(void)
{
    auto dt = M5.Rtc.getDateTime();
    int min = dt.time.minutes;
    if (min == 14 || min == 29 || min == 44 || min == 59)
    {
        //Serial.println("Waiting for quarter.");
        while (dt.time.seconds != 0)
        {
            dt = M5.Rtc.getDateTime();
            delay (TICK_DELAY);
        }
    }
}

/**
 * @brief Build the ISO timestamp (UTC) of the price slot 30 minutes ahead.
 * @param [out] out  Destination buffer, needs at least 25 bytes.
 * @param [in]  size Size of the destination buffer.
 * @return false if the clock is unset or the buffer is too small.
*/
bool makeISODate(char *out, size_t size)
{
    const time_t slotLen = PRICE_INTERVAL * 60;
    time_t now = time(nullptr);

    if (now < 1700000000)   // clock never set, still near 1970
        return (false);

    // Round down to a slot boundary. The grace period means an early
    /// wake-up (e.g. 12:14:40) still lands on the 12:15 slot.
    time_t slot = ((now + EPOCH_OFFSET + SLOT_GRACE) / slotLen) * slotLen;

    struct tm tm;
    gmtime_r(&slot, &tm);

    int len = snprintf(out, size, PRICE_DATE_MASK,
                       tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                       tm.tm_hour, tm.tm_min);

    return (len > 0 && (size_t)len < size);
}

/**
 * @brief Search for date in price list file.
*/
bool fetchPrices(char *isoDate)
{
    Serial.println("Attempting to open price file....");
    File priceListFile = SPIFFS.open(FILE_TEXT_NAME, FILE_READ);
    if (!priceListFile)
    {
        Serial.println("Price file open failed.");
        return (false);
    }

    int lines = 0;
    int priceCount = 0;
    char buffer[40] = {0};
    float tmpFloat = 0.0;
    priceList.priceNow = 99999;
    priceList.pricePlus15 = 99999;
    priceList.pricePlus30 = 99999;

    while (priceListFile.available() && priceCount < 3)
    {
        lines++;
        int bytesRead = priceListFile.readBytesUntil('\n', buffer, sizeof(buffer) - 1);
        buffer[bytesRead] = '\0';
        if (priceCount == 0 && checkDate(buffer, isoDate))
        {
            ++priceCount;
            if (findFloat(buffer, &tmpFloat))
                priceList.pricePlus30 = tmpFloat;
            Serial.print(lines); Serial.print(": buffer: "); Serial.println(buffer);
            continue ;
        }

        switch(priceCount)
        {
        case 1:
            ++priceCount;
            if (findFloat(buffer, &tmpFloat))
                priceList.pricePlus15 = tmpFloat;
            Serial.print(lines); Serial.print(": buffer: "); Serial.println(buffer);
            continue ;
        case 2:
            ++priceCount;
            if (findFloat(buffer, &tmpFloat))
                priceList.priceNow = tmpFloat;
            Serial.print(lines); Serial.print(": buffer: "); Serial.println(buffer);
            continue ;
        default:
            break ;
        }
    }

    Serial.print(lines); Serial.print(": buffer: "); Serial.println(buffer);

    priceListFile.close();

    if (priceCount < 3 || lines <= 40)
    {
        bool res = getJson();
        Serial.print("Old file JSON get, res: "); Serial.println(res);
    }

    if (priceCount < 3)
        return (false);

    return (true);
}

/**
 * @brief Check if a specific date exists in a given string
*/
bool checkDate(char *s, char *d)
{
    char *found = strstr(s, d);
    if (found == NULL)
        return (false);
    return (true);
}

/**
 * @brief Find price in Date,Price line.
 * @details Helper for fetchPrice()
 * strtof stops at the trailing \r
 * A bad line returns false, caller’s existing tmpFloat = 99999 fallback fires instead of the device aborting
*/
bool findFloat(const char *s, float *out)
{
    const char *comma = strchr(s, ',');
    if (comma == NULL)
        return (false);

    char *end = NULL;
    float value = strtof(comma + 1, &end);
    if (end == comma + 1)       // nothing was parsed
        return (false);

    *out = value;
    return (true);
}

/**
 * @brief Fill the display with desired information.
 * @details Price and battery/fault information.
*/
void fillDisplay(void)
{
    //Serial.print("Displaying.... ");
    M5.Display.setEpdMode(epd_mode_t::epd_quality);
    M5.Display.startWrite();    //Batch draw begin

    char buf[64] = {0};
    snprintf(buf, sizeof(buf), "%g", priceList.priceNow); //%g trims off trailling zeroes
    M5.Display.clear(TFT_WHITE);
    M5.Display.setTextSize(TEXT_SIZE);
    M5.Display.drawCenterString(buf, 100, 20);
    snprintf(buf, sizeof(buf), "%g", priceList.pricePlus15);
    M5.Display.drawCenterString(buf, 100, 90);
    snprintf(buf, sizeof(buf), "%g", priceList.pricePlus30);
    M5.Display.drawCenterString(buf, 100, 145);
    
    //Fill error string with catalogued errors.
    //Check RTC for voltage dropout and if occured, set potential fault flag.
    //Print 'BAT LOW' if battery level goes below 10%
    char errorString[5] = "    ";
    if (M5.Rtc.getVoltLow())
        errorString[0] = 'V';
    if (errorClock)
        errorString[1] = 'C';
    if (errorJSON)
        errorString[2] = 'J';
    if (errorFile)
        errorString[3] = 'F';
    char *batRecharge = "       ";
    if (M5.Power.getBatteryLevel() < 10)
        batRecharge = "BAT LOW";

    auto dt = M5.Rtc.getDateTime();

    M5.Display.setTextSize(TEXT_SIZE * 0.278);
    snprintf(buf, sizeof(buf), "Bat: %d - %s %s           %02d:%02d", M5.Power.getBatteryLevel(), errorString, batRecharge, dt.time.hours + 2, dt.time.minutes);
    M5.Display.drawString(buf, 0, 0);

    M5.Display.endWrite();
    M5.Display.waitDisplay();
    //Serial.println("Finished.");
}

/**
 * @brief Calculate time to sleep in seconds
 * @note Maybe not anymore--Returns roughly a minute longer than the needed time.
 *       However this seems to work for this particular device.
*/
int calculateSleepTime(void)
{
    int restMins = 0;
    int restSex = 0;

    auto dt = M5.Rtc.getDateTime();
    int cm = dt.time.minutes;

    if (cm >= 0 && cm < 15)
        restMins = 14 - cm;
    else if(cm >= 15 && cm < 30)
        restMins = 29 - cm;
    else if (cm >= 30 && cm < 45)
        restMins = 44 - cm;
    else if (cm >= 45)
        restMins = 59 - cm;

    restSex = 60 - dt.time.seconds;

    //Serial.print("Rest minutes: "); Serial.print(restMins); Serial.print(" | Rest seconds: "); Serial.println(restSex);
    return ((restMins * 60) + restSex);
}

/**
 * @brief Calculate sleep time and put device to sleep
 * @details deepSleep seems to not work on this device when it is unplugged.
 *          powerOff does not work while the device is plugged in, but seems OK when unplugged.
*/
void goToSleep(void)
{
    int restSeconds = calculateSleepTime();

    Serial.println("Sleep function initialised.");
    M5.Display.sleep();
    M5.Rtc.clearIRQ();
    M5.Rtc.setAlarmIRQ(restSeconds + SLEEP_OFFSET);

    esp_sleep_enable_timer_wakeup((restSeconds + 2) * 1000000);

    M5.Power.powerOff();
    Serial.println("powerOff() fired, this should not display.");
    // battery: power cut, RTC alarm powers the device on (timer is moot)
    // USB:     ESP32 deep-sleeps, timer wakes it into a fresh boot


    /*int restSeconds = calculateSleepTime();
    
    Serial.println("Sleep function initialised.");
    M5.Display.sleep();
    M5.Rtc.clearIRQ();
    M5.Rtc.setAlarmIRQ(restSeconds + SLEEP_OFFSET);
    M5.Power.powerOff();
    Serial.println("powerOff() fired");

    // Only reached when USB powered
    delay(500);

    while(!M5.Rtc.getIRQstatus())
        delay(1000);

    ESP.restart();*/
}

/**
 * @brief Display files on filesystem among other filesystem data
*/
void displayFiles(void)
{
    Serial.println("\r\nFilesystem informaton.");
    Serial.print("Size: "); Serial.println(SPIFFS.totalBytes());
    Serial.print("Used: "); Serial.println(SPIFFS.usedBytes());
    Serial.print("\r\n");
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    while(file)
    {
        Serial.print("FILE: ");
        Serial.print(file.name());
        Serial.print(" | Size: ");
        Serial.print(file.size());
        Serial.print(" | Edited time: ");
        Serial.println(file.getLastWrite());
        file.close();
        file = root.openNextFile();
    }
    file.close();
    root.close();
}

/**
 * @brief Attempts to extract a float from a string.
 * @param [in] const char *c_str
 */
/*float formatText(const char *c_str)
{
    String number;
    float outNum;

    while (!std::isdigit(*c_str) && *c_str != 45)
        c_str++;
    if (*c_str == 45)
    {
        number = number + *c_str;
        c_str++;
    }
    while ((*c_str > 47 && *c_str < 58) || *c_str == 46)
    {
        number = number + *c_str;
        c_str++;
    }

    outNum = std::stof(number.c_str());
    return (outNum);
}*/
