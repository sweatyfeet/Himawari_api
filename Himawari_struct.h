#ifndef HIMAWARI_STRUCT_H
#define HIMAWARI_STRUCT_H

typedef struct REQUEST_PARAM		// Keys for hourly requests.
{
	std::string apiKey;
	std::string wkt;
	std::string wktLon;
	std::string wktLat;

	std::string attributes;
	std::string names;
	std::string utc;
	std::string leapDay;
	std::string interval;

	std::string siteName;
	std::string email;

	std::string affiliation;
	std::string reason;
	std::string mailingList;

}request_param;



std::string resAttName[24] = {

/*
	List of attribute names; The following six attributes of
	'alpha', 'aod', 'asymmetry', 'cld_opd_dcmop', 'cld_reff_dcmop', 'ssa'
	are currently unavailable in requests.

	Names of attributes here are derived from the actual .csv files,
	not the developer.nrel.gov himawari info page.
*/

	"Temperature", "alpha", "aod",							
	"asymmetry", "cld_opd_dcomp", "cld_reff_dcomp",			
	"Clearsky DHI", "Clearsky DNI", "Clearsky GHI",			
	"Cloud Type", "Dew Point", "DHI",						
	"DNI", "Fill Flag", "GHI",								
	"Ozone", "Relative Humidity", "Solar Zenith Angle",		
	"ssa", "Surface Albedo", "Pressure",					
	"Precipitable Water", "Wind Direction", "Wind Speed"	
};

typedef struct RESPONSE_HOURLY
{
	int year;
	int month;
	int day;
	int hour;
	int minute;

	double attValue[24] = {
		-999.0, -999.0, -999.0, -999.0, -999.0, -999.0,
		-999.0, -999.0, -999.0, -999.0, -999.0, -999.0,
		-999.0, -999.0, -999.0, -999.0, -999.0, -999.0,
		-999.0, -999.0, -999.0, -999.0, -999.0, -999.0
	};
}response_hourly;

typedef struct RESPONSE_YEARLY
{
	std::string siteName;

	int year;
	double lat;
	double lon;
	double elev;
	int utcZone;

	int attNum[24];

	std::list<response_hourly> responseSet;
}response_yearly;



typedef struct DSSAT_DAILY		// Structs for dssat input.

{
	int doy;
	double srad;
	double tmax;
	double tmin;
	double rain;
	double dewp;
	double wind;
	double rhum;
}dssat_daily;

typedef struct DSSAT_YEARLY
{
	std::string wthCode;
	int year;

	double lat;
	double lon;
	double elev;
	double tav;
	double range;

	double refht = -99.0;
	double wndht = -99.0;

	std::list<dssat_daily> dssatSet;
}dssat_yearly;

#endif
