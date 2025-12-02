A utility that fetches the current electricity spot price from porssisahko.net. So I will always have the current electricity price on my desktop. 

I use the Mate command applet, but it doesn't require that.

script/script.sh - Shell script to fetch the current price every hour.

desktop/ - Small program to fetch the current price list and output the current price.
The price list updates every day at 1400.
Uses external date tool by Howard Hinnant and rapidjson.

Updated for October 1st's rollout of 15 minute price updates.
