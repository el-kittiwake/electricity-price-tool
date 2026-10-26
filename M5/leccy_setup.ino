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
 * @brief Setup function, runs once.
 * @details In our case device shuts down for a set period after this runs.
            Loop function does not run.
*/
void setup(void) {
    //Serial init
    serialInit();

    //Device init
    deviceInit();

    //Attempt to mount filesystem.
    //Serial.println("Mounting filesystem....");
    if (!SPIFFS.begin(true))
    {
        //Serial.println("Filesystem mount Failed!");
        errorFile = true;
    }

    if (!SPIFFS.exists(FILE_TEXT_NAME))
    {
        //Serial.println("Getting JSON....");
        if (!getJson())
        {
            //Serial.println("Get JSON function failed!");
            errorJSON = true;
        }
        //Serial.println("Get JSON function done.");
    }
    //else
        //Serial.println("Prices file already present.");

    //In the event of waking up too soon, wait for the right time
    waitForQuarter();

    //Fetch current and next 2 prices
    char isoDate[25] = {0};
    if (!makeISODate(isoDate, sizeof(isoDate)))
        errorClock = true;
    else if (!fetchPrices(isoDate))
        errorFile = true;
    Serial.print("ISOdate made: "); Serial.println(isoDate);
    Serial.print("Prices fetched: Now: "); Serial.print(priceList.priceNow);
    Serial.print(" | In 15: "); Serial.print(priceList.pricePlus15);
    Serial.print(" | In 30: "); Serial.println(priceList.pricePlus30);

    //Fill display with desired information
    fillDisplay();

    //Display file list
    //displayFiles();

    //Put to sleep until the next quarter update
    goToSleep();
}
