#pragma once

#include <fstream>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <ratio>
#include <cstdint>				//Integer wrappers
#include <cstdlib>				//Exit status definitions
#include <map>
#include "../include/rapidjson/document.h"
#include "../include/date.h"	//External header, for ISO time function
#include <curl/curl.h>
#include <curl/easy.h>

using namespace rapidjson;
using namespace std::chrono;

class leccy
{
private:
	//map:- index: start time, value: c/kWh
	std::map<std::string, float> latestPrices_;

	std::string jsonFile_ = "latest-prices.json";
	const char *jsonFileAddr_ = "https://api.porssisahko.net/v2/latest-prices.json";
	
	int8_t intervalTimeMinutes_ = 15;
	duration<int64_t, std::milli> intervalMilliSeconds_;

	void readData_(void);
	bool tryGetJSONFile_(void);
	bool arePricesOld_(void);
	
	void printParamHelp_(void);
	void printData_(void);
	void printCurrentPrice_(void);
	
	/*
		Templated functions
	*/
	//https://stackoverflow.com/questions/9527960/how-do-i-construct-an-iso-8601
	//Uses date.h from: https://howardhinnant.github.io/date/date.html
	//May not be suitable for later use.
	template <class Precision>
	std::string getISOCurrentTimestamp_(void)
	{
		auto now = system_clock::now();
		return date::format("%FT%TZ", date::floor<Precision>(now));
	}

public:
	~leccy() = default;	
	leccy(void);

	void chooseAction(const char *param);
};