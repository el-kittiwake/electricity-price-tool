#include "leccy.hpp"

leccy::leccy(void)
{
	if (std::ifstream(jsonFile_).good())
	{
		readData_();
		if (arePricesOld_())
		{
			if (!tryGetJSONFile_())
				throw std::runtime_error("JSON file not fetched (1).");
			readData_();
		}
	}
	else
	{
		if (!tryGetJSONFile_())
			throw std::runtime_error("JSON file not fetched (2).");
		readData_();
	}
}

/**
 * @brief Read json file into string and parse into map.
 * @details Fill string with whole file contents. Convert to cstring.
 * https://en.cppreference.com/w/cpp/iterator/istreambuf_iterator.html
 * https://www.reddit.com/r/learnprogramming/comments/3qotqr/
 * @details Use RapidJSON library to parse JSON into map. New to me.
 * Create a Value containing the contents of the "prices" array in the JSON.
 * Iterate through the Value, create 2 iterators at the locaton of "startDate"
 * and "prices" and if values are present fill the latestPrices_ map with the
 * values of those iterators.
 * https://rapidjson.org/md_doc_tutorial.html#QueryValue
 * I could probably do without the map.
 */
void leccy::readData_(void)
{
	std::ifstream pricesFile(jsonFile_);
	if (!pricesFile.is_open())
		throw std::invalid_argument("Unable to open " + jsonFile_);

	std::string fileContents
	{
		std::istreambuf_iterator<char>(pricesFile), std::istreambuf_iterator<char>()
	};
	pricesFile.close();

	const char *cstrContents = fileContents.c_str();
	Document jsonDOM;
	jsonDOM.Parse(cstrContents);

	/**
	 * latest-prices.json stores each quarter hour price in an array called
	 * "prices". This line sets a Value to that array.
	 */
	const Value& quarters = jsonDOM["prices"];

	for (uint32_t i = 0; i < quarters.Size(); i++)
	{
		Value::ConstMemberIterator itrDate = quarters[i].FindMember("startDate");
		Value::ConstMemberIterator itrPrice = quarters[i].FindMember("price");
		if (itrDate != jsonDOM.MemberEnd())
		{
			std::pair<std::string, float> tmpPair = std::make_pair(itrDate->value.GetString(), itrPrice->value.GetFloat());
			latestPrices_.insert(tmpPair);
		}
		else
			throw std::runtime_error("JSON parsing failure.");
	}
}

/**
 * @brief Try to GET the latest-prices JSON file from remote site.
 * Only if there is no stored JSON file or if one is past the 1400 update schedule.
 * @details Using libcurl, easy functions.
 * https://curl.se/libcurl/c/simple.html
 * https://stackoverflow.com/questions/25934409/save-a-file-with-a-absolute-path
 * 
 */
bool leccy::tryGetJSONFile_(void)
{
	if (!arePricesOld_())
		return (true);

	CURL *curl;
	CURLcode res;
	FILE *gotFile;
	
	curl = curl_easy_init();
	if(curl)
	{
		gotFile = fopen(jsonFile_.c_str(), "w");
		curl_easy_setopt(curl, CURLOPT_URL, jsonFileAddr_);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, gotFile);
		
		/* Perform the request, res gets the return code */
		res = curl_easy_perform(curl);
		//Cleanup
		curl_easy_cleanup(curl);
		fclose(gotFile);
		/* Check for errors */
		if(res != CURLE_OK)
		{
			std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
			return (false);
		}
		return (true);
	}
	return (false);
}

/**
 * @brief Check how old the current prices are.
 * @details Also check that latestPrices_ exists.
 * @return True: if prices have not been updated today or there are no prices.
 */
bool leccy::arePricesOld_(void)
{
	if (latestPrices_.size() <= 0)
		return (true);

	std::time_t now;
	time(&now);
	std::tm *currTime = localtime(&now);
	//std::cout << currTime->tm_mday << " | " << currTime->tm_hour << std::endl;

	auto tmpEnd = latestPrices_.end();
	tmpEnd--;
	if (currTime->tm_mday >= std::stoi(tmpEnd->first.substr(8, 2)) && currTime->tm_hour >= 16)
	{
		//std::cout << "Day from map: " << std::stoi(tmpEnd->first.substr(8, 2)) << std::endl;
		return (true);
	}

	return (false);
}

/**
 * @brief Take input parameter and perform appropriate action from it.
 * @param param: n: print current price as a single floating point value.
 */
void leccy::chooseAction(const char *param)
{
	if (!strcmp(param, "-n") || !strcmp(param, "--now"))
		printCurrentPrice_();
	else if (!strcmp(param, "-a") || !strcmp(param, "--all"))
		printData_();
	else if (!strcmp(param, "--help"))
		printParamHelp_();
	else
	{
		std::cout << "Invalid parameter\n";
		printParamHelp_();
	}
}

/**
 * @brief Print available parameters and usage.
 */
void leccy::printParamHelp_(void)
{
	std::cout << "Usage: ./leccy <param>\n";
	std::cout << "\t-n\t--now\t\tPrint the price at the current time.\n";
	std::cout << "\t-a\t--all\t\tPrint all the prices in the current list.\n";
	std::cout << std::endl;
}

/**
 * @brief Print entire contents of the latestPrices_ map. With a header.
 */
void leccy::printData_(void)
{
	std::cout << "        startDate        " << "| price" << "\n";

	for (auto i : latestPrices_)
		std::cout << i.first << " | " << i.second << "\n";

	std::cout << std::endl;
}

/**
 * @brief Print current price as a single floating point value.
 * @details 
 */
void leccy::printCurrentPrice_(void)
{
	std::string currTime = getISOCurrentTimestamp_<std::chrono::milliseconds>();

	//C++ string operations make this very easy
	auto i = latestPrices_.lower_bound(currTime);
	if (i != latestPrices_.begin() && i->first != currTime)
		i--;
	float closestValue = i->second;

	std::cout << closestValue << std::endl;
}
