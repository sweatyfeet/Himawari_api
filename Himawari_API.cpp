#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <list>
#include "curl/curl.h"
#include "Himawari_struct.h"
#include "Himawari_class.h"

Himawari::Himawari()
{
	std::cout << "[Report] Initializing Himawari_API.cpp..." << std::endl << std::endl;
}

Himawari::~Himawari()
{
	std::cout << std::endl << "[Report] Program finished. Terminating..." << std::endl;
}

void Himawari::getRequestParam(std::ifstream& reqParam)
{
	std::string ifName;
	std::string apiKey, wkt, wktLon, wktLat, att,
		year, utc, leapDay, interval, siteName, email;

	char attBuf[128];

	std::stringstream ss;
	char buffer[512];

	std::cout << "[Input] Enter input file name: ";
	std::cin >> ifName;
	std::cin.ignore();
	reqParam.open(ifName);

	while (reqParam.fail())
	{
		ifName.clear();
		std::cout << "[Input] Unavailable file name. Please try again: ";
		std::cin >> ifName;
		std::cin.ignore();
		reqParam.open(ifName);
	}

	reqParam.getline(buffer, 512, '\n');       // Dump file header
	ss << buffer;
	ss.str("");
	ss.clear();

	while (reqParam.good())
	{
		reqParam.getline(buffer, 512, '\n');
		for (int index = 0; index < reqParam.gcount(); index++)
		{
			if (buffer[index] == ',')
			{
				buffer[index] = ' ';
			}
		}

		ss << buffer;

		ss >> apiKey;

		ss >> wkt;
		ss >> wktLon;
		ss >> wktLat;
		wkt.append("(").append(wktLon).append("+").append(wktLat).append(")");

		ss >> attBuf;
		for (int index = 0; index < 128; index++)
		{
			if (attBuf[index] == '&')
			{
				attBuf[index] = ',';
			}
		}
		
		att = attBuf;
		ss >> year;
		ss >> utc;
		ss >> leapDay;
		ss >> interval;
		ss >> siteName;
		ss >> email;

		ss.str("");
		ss.clear();

		std::cout << "[Report] Acquired the following query data: " <<
			wkt << ' ' << att << ' ' << year << ' ' << utc << ' ' << leapDay << ' '
			<< interval << ' ' << siteName << ' ' << email << std::endl;

		request_param param;

		// Storing request parameteres in class member.
		param.apiKey = apiKey;
		param.wkt = wkt;
		param.attributes = att;
		param.names = year;
		param.utc = utc;
		param.leapDay = leapDay;
		param.interval = interval;
		param.siteName = siteName;
		param.email = email;

		requestList.push_back(param);
	}
	reqParam.close();
	std::cout << "[Report] File input successful." << std::endl << std::endl;
}

void Himawari::sendRequest()
{
	// Global curl initiation.
	curl_global_init(CURL_GLOBAL_ALL);

	// Initiation of handles.
	CURLcode errMsg;
	CURLUcode urlErrMsg;
	CURL* handle = curl_easy_init();
	CURLU* urlHandle = curl_url();

	// Enabling verbose messages.
	// errMsg = curl_easy_setopt(handle, CURLOPT_VERBOSE, 1L);

	// The scheme MUST be set to https
	urlErrMsg = curl_url_set(urlHandle, CURLUPART_SCHEME, "https", 0);

	// Providing user-agent field in advance. 
	curl_easy_setopt(handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

	// Enabling redirection; csv files are downloaded from the redirected url.
	errMsg = curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);

	// Building base url.
	urlErrMsg = curl_url_set(urlHandle, CURLUPART_HOST, "developer.nrel.gov", 0);

	// Data chuck of a single response.
	std::string chunk;

	// Setting a callback function & pointer to where the data is saved.
	errMsg = curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, saveResponse);
	errMsg = curl_easy_setopt(handle, CURLOPT_WRITEDATA, &chunk);


	// Building query & requesting & saving response.

	std::list<request_param>::iterator iter;
	iter = requestList.begin();
	for (; iter != requestList.end(); iter++)
	{
		appendQuery("api_key=", iter->apiKey, urlHandle);
		appendQuery("wkt=", iter->wkt, urlHandle);
		appendQuery("attributes=", iter->attributes, urlHandle);
		appendQuery("names=", iter->names, urlHandle);
		appendQuery("utc=", iter->utc, urlHandle);
		appendQuery("leap_day=", iter->leapDay, urlHandle);
		appendQuery("interval=", iter->interval, urlHandle);
		appendQuery("full_name=", iter->siteName, urlHandle);
		appendQuery("email=", iter->email, urlHandle);

		int year = std::stoi(iter->names);
		if (year <= 2015) {
			urlErrMsg = curl_url_set(urlHandle, CURLUPART_PATH, "/api/nsrdb/v2/solar/himawari7-download.csv", 0);
		}
		else {
			urlErrMsg = curl_url_set(urlHandle, CURLUPART_PATH, "/api/nsrdb/v2/solar/himawari-download.csv", 0);
		}

		// Assigning query to the handle.
		std::string temp;
		char* reqUrl = nullptr;
		int resCode = 0;

		urlErrMsg = curl_url_get(urlHandle, CURLUPART_URL, &reqUrl, 0);
		errMsg = curl_easy_setopt(handle, CURLOPT_URL, reqUrl);
		
		temp = reqUrl;
		std::cout << "[Report] Sending request to the following: " << temp << std::endl << std::endl;



		errMsg = curl_easy_perform(handle);
		
		if (errMsg != CURLE_OK)
			std::cout << "[Error] Curl request failed: " << curl_easy_strerror(errMsg) << std::endl;
		else
		{
			curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, resCode);
			std::cout << "[Report] Response code = " << resCode << "." << std::endl;
			std::cout << "[Report] Request successful for a dataset." << std::endl;

			// Parsing csv data in the memory.
			parseResponse(chunk, iter, responseList);
			std::cout << "[Report] Response data saved: "
				<< iter->siteName << "_" << iter->names << std::endl << std::endl;
		}

		// Cleaning up previous data.
		chunk.clear();	
		urlErrMsg = curl_url_set(urlHandle, CURLUPART_QUERY, nullptr, 0);
	}

	// Cleaning up curl memory
	curl_easy_cleanup(handle);
	curl_url_cleanup(urlHandle);
	curl_global_cleanup();
}

size_t Himawari::saveResponse(void* passedPtr, size_t unitSize,
	size_t chunkSize, void* userMemory)
{
	// Getting total size of response data.
	size_t totalSize = unitSize * chunkSize;

	// Declaring a temporary std::string pointer.
	auto& content = *static_cast<std::string*>(userMemory);

	// Save response data.
	content.append(static_cast<char*>(passedPtr), totalSize);

	return totalSize;
}

void Himawari::appendQuery(const char* key, std::string value, CURLU* urlHandle)
{
	std::string query = key;
	query.append(value);
	curl_url_set(urlHandle, CURLUPART_QUERY, query.c_str(), CURLU_APPENDQUERY);
}

void Himawari::parseResponse(std::string& chunk, std::list<request_param>::iterator iter,
	std::list<response_yearly> &responseList)
{
	response_yearly resYr;
	std::stringstream whole, line, dump;
	char lineBuf[1024], attBuf[64];

	resYr.siteName = iter->siteName;
	resYr.year = stoi(iter->names);

	whole << chunk;

	whole.getline(lineBuf, 1024, '\n');     // Dumping first line.
	dump << lineBuf;
	dump.str("");
	dump.clear();

	whole.getline(lineBuf, 1024, '\n');		// Parsing geographic data from second line.
	line << lineBuf;

	for (int count = 0; count < 5; count++)
	{
		line.getline(attBuf, 64, ',');
		dump << attBuf;
	}

	line.getline(attBuf, 64, ',');	resYr.lat = std::stod(attBuf);		dump << attBuf;
	line.getline(attBuf, 64, ',');	resYr.lon = std::stod(attBuf);		dump << attBuf;
	line.getline(attBuf, 64, ',');	dump << attBuf;
	line.getline(attBuf, 64, ',');	resYr.elev = std::stod(attBuf);		dump << attBuf;
	line.getline(attBuf, 64, ',');	resYr.utcZone = std::stoi(attBuf);	dump << attBuf;
	
	line.str("");
	line.clear();
	dump.str("");
	dump.clear();


	whole.getline(lineBuf, 1024, '\n');		// Saving parameter names in third line.
	line << lineBuf;

	for (int count = 0; count < 5; count++)
	{
		line.getline(attBuf, 64, ',');
		dump << attBuf;
	}
	dump.str("");
	dump.clear();



	for (int count = 0; !line.eof(); count++)
	{
		line.getline(lineBuf, 64, ',');
		std::string attName = lineBuf;
		
		int index = 0;
		while (attName != resAttName[index]) index++;
		resYr.attNum[count] = index;

		dump << lineBuf;
		dump.str("");
		dump.clear();
	}
	line.str("");
	line.clear();


	while (!whole.eof())					// Parsing the rest of data.
	{
		response_hourly resHr;

		whole.getline(lineBuf, 1024, '\n');
		for (int count = 0; count < whole.gcount(); count++)
		{
			if (lineBuf[count] == ',')
			{
				lineBuf[count] = ' ';
			}
		}

		line << lineBuf;
		line >> resHr.year;
		line >> resHr.month;
		line >> resHr.day;
		line >> resHr.hour;
		line >> resHr.minute;

		for (int count = 0; !line.eof(); count++)
			line >> resHr.attValue[resYr.attNum[count]];

		line.str("");
		line.clear();

		resYr.responseSet.push_back(resHr);
	}

	resYr.responseSet.pop_back();
	responseList.push_back(resYr);
}

void Himawari::epwOutput(std::ofstream &epw)
{
	std::list<response_yearly>::iterator yiter;
	yiter = responseList.begin();

	

	for (; yiter != responseList.end(); yiter++)
	{
		std::string ofName = yiter->siteName;
		ofName.append("_").append(std::to_string(yiter->year)).append(".epw");

		epw.open(ofName);
		if (epw.fail())
		{
			ofName.clear();
			std::cout << "[Error] Unable to open epw output file. Skipping current data set." << std::endl;
			continue;
		}

		// Printing file header.
		
		// Geograpical info: latitude, longitude, timezone , elevation
		int timezone = round((yiter->lon) / 15.0) + 1;

		epw << "LOCATION," << yiter->siteName << ",-,-,MN7,999,"
			<< yiter->lat << "," << yiter->lon << "," << timezone << "," << yiter->elev << std::endl;

		epw << "DESIGN CONDITIONS,0\n" << "TYPICAL/EXTREME PERIODS,0\n"
			<< "GROUND TEMPERATURES,1,1.0,,,,10.1,7.4,6.4,7.4,10.1,13.8,17.5,20.1,21.1,20.1,17.5,13.8\n"
			<< "HOLIDAYS/DAYLIGHT SAVINGS,No,0,0,0\n"
			<< "COMMENTS 1,  METEONORM Version 8.0.2.24236\n"
			<< "COMMENTS 2,\n"
			<< "DATA PERIODS,1,1,Data,Sunday,1/1,12/31" << std::endl;

		

		std::list<response_hourly>::iterator hiter;
		hiter = (yiter->responseSet).begin();
		for (; hiter != (yiter->responseSet).end(); hiter++)
		{
			epw << hiter->year << "," << hiter->month << "," << hiter->day << ","
				<< hiter->hour << "," << hiter->minute << "," << yiter->siteName << ",";

			// Printing each of the attributes.				// Name in csv (units conversion)
			epwCheckNull(hiter->attValue[0], epw, 0);		// Temperature ('C)
			epwCheckNull(hiter->attValue[10], epw, 0);		// Dewpoint ('C)
			epwCheckNull(hiter->attValue[16], epw, 0);		// Relative humidity (%)
			epwCheckNull(hiter->attValue[20], epw, 20);		// Atmos. pressure (mbar -> Pa)

			epw << "-999,-999,-999,";

			epwCheckNull(hiter->attValue[14], epw, 0);		// GHI (W/m2 -> Wh/m2)
			epwCheckNull(hiter->attValue[12], epw, 0);		// DNI (W/m2 -> Wh/m2)
			epwCheckNull(hiter->attValue[11], epw, 0);		// DHI (W/m2 -> Wh/m2)

			epw << "-999,-999,-999,-999,";
			
			epwCheckNull(hiter->attValue[22], epw, 0);		// Wind direction (degree)
			epwCheckNull(hiter->attValue[23], epw, 0);		// Wind speed (m/s)

			epw << "-999,-999,-999,-999,-999,-999,";

			epwCheckNull(hiter->attValue[21], epw, 21);		// Precip. water (cm -> mm)
			
			epw << "-999,-999,-999,-999,-999,-999";
			epw << std::endl;
		}

		epw.close();
		epw.clear();
		std::cout << "[Report] Epw parsed: " << ofName << std::endl;
	}

	std::cout << "[Report] Epw parsing finished." << std::endl << std::endl;
}

void Himawari::epwCheckNull(double value, std::ofstream &epw, int flag)
{
	switch (flag)
	{
	case 20:					// Atmos. pressure (mbar -> Pa)
		if (value != -999) epw << value * 100 << ",";
		else epw << "-999,";
		break;
	case 21:					// Precip. water (cm -> mm)
		if (value != -999) epw << value * 10 << ",";
		else epw << "-999,";
		break;
	default:					// No unit conversion
		if (value != -999) epw << value << ",";
		else epw << "-999,";
	}
}

void Himawari::calcDaily()
{
	std::list<response_yearly>::iterator yiter;
	yiter = responseList.begin();

	for (; yiter != responseList.end(); yiter++)
	{
		dssat_yearly dssatYr;
		double tavYr = 0.0;
		double tavMon = 0.0;
		double tavDay = 0.0;

		int prevMon = 1;
		int currMon = 1;
		int dayCount = 0;

		double warmMon = -99.0;
		double coldMon = 99.0;
		double range;
		
		dssatYr.wthCode = yiter->siteName;
		dssatYr.year = yiter->year;

		dssatYr.lat = yiter->lat;
		dssatYr.lon = yiter->lon;
		dssatYr.elev = yiter->elev;
		
		

		std::list<response_hourly>::iterator hiter;
		hiter = (yiter->responseSet).begin();

		for (; hiter != (yiter->responseSet).end();)
		{
			int doy;
			double srad = 0.0;
			double tmax = -99.0;
			double tmin = 99.0;
			double rain = 0.0;
			double dewp = 0.0;
			double wind = 0.0;
			double rhum = 0.0;

			doy = calcDoy(hiter->year, hiter->month, hiter->day);

			for (int count = 0; count < 24; count++, hiter++)
			{
				srad += hiter->attValue[14] * 3600 * 0.000001;	// Sum of daily radiation (W/m2 -> MJ/m2).
				
				if (tmax < hiter->attValue[0]) tmax = hiter->attValue[0];	// Daily max. temperature.
				if (tmin > hiter->attValue[0]) tmin = hiter->attValue[0];	// Daily min. temp.

				rain += hiter->attValue[21] * 10;		// Sum of daily prep. water (cm -> mm)
				
				dewp += hiter->attValue[10];
				wind += hiter->attValue[23] * 86.4;		// Wind speed (m/s -> km/day)
				rhum += hiter->attValue[16];

				tavDay += hiter->attValue[0];			// Sum of hourly temperatures.
			}

			dewp /= 24.0;		// Daily average of dewpoint, wind speed, rel. humidity.
			wind /= 24.0;
			rhum /= 24.0;

			dssat_daily dssatDay;

			dssatDay.doy = doy;
			dssatDay.srad = srad;
			dssatDay.tmax = tmax;
			dssatDay.tmin = tmin;
			dssatDay.rain = rain;
			dssatDay.dewp = dewp;
			dssatDay.wind = wind;
			dssatDay.rhum = rhum;

			dssatYr.dssatSet.push_back(dssatDay);

			

			// Calculate tav (Average anuual temperature) & amp (Average annual temp. range).

			tavDay /= 24.0;
			tavMon += tavDay;
			tavDay = 0.0;
		
			if (hiter == (yiter->responseSet).end()) currMon = 0;
			else currMon = hiter->month;

			dayCount++;

			if (prevMon != currMon)
			{
				prevMon = currMon;
				tavMon /= dayCount;
				tavYr += tavMon;

				if (tavMon > warmMon) warmMon = tavMon;
				if (tavMon < coldMon) coldMon = tavMon;

				tavMon = 0.0;
				dayCount = 0;
			}
		}

		tavYr /= 12.0;
		dssatYr.tav = tavYr;

		range = warmMon - coldMon;
		dssatYr.range = range;
		
		dssatList.push_back(dssatYr);
	}

	std::cout << "[Report] Acquired daily data set." << std::endl << std::endl;
}

int Himawari::calcDoy(int year, int month, int day)
{
	// Montly DOY (Non-leap Year)
	// 1  2  3  4   5   6   7   8   9   10  11  12
	// 31 28 31 30  31  30  31  31  30  31  30  31
	//    59 90 120 151 181 212 243 273 304 334 365

	int doyList[12] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
	day += doyList[month - 1];

	// Accounting for leap years.
	if ((year % 4 == 0) && month >= 3)
		day += 1;

	if (year > 2000) day += (year - 2000) * 1000;
	else day += (year - 1900) * 1000;

	return day;
}

void Himawari::wthOutput(std::ofstream& wth)
{
	std::list<dssat_yearly>::iterator yiter;
	yiter = dssatList.begin();

	const int cellSize = 6;
	const int latLonSize = 9;
	const int doySize = 5;

	for (; yiter != dssatList.end(); yiter++)
	{
		std::string ofName = yiter->wthCode;
		ofName.append("_").append(std::to_string(yiter->year)).append(".wth");

		wth.open(ofName);
		if (wth.fail())
		{
			ofName.clear();
			std::cout << "[Error] Unable to open epw output file. Skipping current data set." << std::endl;
			continue;
		}



		// Printing file header.
		
		wth << "*WEATHER DATA : " << yiter->wthCode << "\n\n"
			<< "@ INSI      LAT     LONG  ELEV   TAV   AMP REFHT WNDHT" << std::endl;

		wth << std::setw(cellSize) << std::right << (yiter->wthCode).substr(0, 4)	// 'Ui64' should be erased in linux systems.
			<< std::setw(latLonSize) << std::right << yiter->lat		
			<< std::setw(latLonSize) << std::right << yiter->lon		
			<< std::setw(cellSize) << std::right << yiter->elev		
			<< std::setw(cellSize) << std::right << std::fixed << std::setprecision(2) << yiter->tav		
			<< std::setw(cellSize) << std::right << yiter->range	
			<< std::setw(cellSize) << std::right << std::fixed << std::setprecision(1) << yiter->refht	
			<< std::setw(cellSize) << std::right << yiter->wndht	<< std::endl;

		wth << "@DATE  SRAD  TMAX  TMIN  RAIN  DEWP  WIND  RHUM" << std::endl;

		

		std::list<dssat_daily>::iterator diter;
		diter = (yiter->dssatSet).begin();

		for (; diter != (yiter->dssatSet).end(); diter++)
		{
			wth << std::setw(doySize) << std::right << std::setfill('0') << diter->doy;
			wth	<< std::fixed << std::setprecision(1) << std::right << std::setfill(' ');

			wth << std::setw(cellSize) << diter->srad
				<< std::setw(cellSize) << diter->tmax
				<< std::setw(cellSize) << diter->tmin
				<< std::setw(cellSize) << diter->rain
				<< std::setw(cellSize) << diter->dewp
				<< std::setw(cellSize) << diter->wind
				<< std::setw(cellSize) << diter->rhum << std::endl;
		}

		wth.close();
		wth.clear();

		std::cout << "[Report] Wth parsed: "
			<< (yiter->wthCode).append("_").append(std::to_string(yiter->year)) << std::endl;
	}

	std::cout << "[Reort] Wth parsing finished." << std::endl;
}

int main()
{
	Himawari himawari;
	std::ifstream reqParam;
	std::ofstream epw, wth;

	himawari.getRequestParam(reqParam);
	himawari.sendRequest();
	himawari.epwOutput(epw);

	himawari.calcDaily();
	himawari.wthOutput(wth);

	return 0;
}