#!/bin/bash

# Simple script to update the mate command applet to show the current electricity spot price. API used: https://porssisahko.net/api
# Updated 30th Sept to reflect 15 minute prices starting Oct 1st.
# Spotprice + marginal is what we pay. Price is good when less than contract price.
# Sept 30 2025: https://hehkuenergia.fi/ekoenergia/
# MARGINAL=0.69
# KK12=8.49 #(7.8)
# KK24=8.19 #(7.5)

# Variable collection
# Using v2 (15 minute prices)
DATE=$(date -I)
#HOUR=$(date +%H) # Was only used for v1
MINUTE=$(date +%M)

TIME="T"$(date --utc +%T.%N)"Z"
DATE=${DATE}$TIME

#echo $DATE
#echo $HOUR
#echo $TIME
#echo $MINUTE

# Use curl to fetch the current price from porssisahko.net but only at quarter
# bells. Write to file. Any other times, fetch file.
if [[ $MINUTE == 00 || $MINUTE == 15 || $MINUTE == 30 || $MINUTE == 45 ]];
then
    PRICE="$(curl -s https://api.porssisahko.net/v2/price.json\?date\=$DATE)"
	echo $PRICE > ~/spot_price
fi

# sed away the JSON formatting to temporary variable
TMP="$(echo "$(cat ~/spot_price)" | sed "s|{\"price\":||g" | sed "s|}||g")"

# output the trimmed price
echo $TMP
