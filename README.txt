<COMPILE OPTIONS>
g++ -o curltest -o curltest.cpp -lcurl -Idl -lz -static

- All four of the complie options are required.


<INPUT FILE FORMAT>
- Input file name will be requested to user interactively.

/*
(header)
(APIKEY),(wktCase),(longitude),(latitude),(chain of attributes),(year),(utc = true / local time = false), (whether or not to count leap day),(Data time interval),(Data name),(User email)
*/

wktCase,wktLon,wktLat,attributes,year,utc,leap_day,interval,dataName,email
(APIKEY),POINT,127.117,35.841,air_temperature&dew_point&relative_humidity&wind_speed&total_precipitable_water&ghi&dni&dhi,2019,true,true,60,Jeon,jjmin416@snu.ac.kr
(APIKEY),POINT,127.117,35.841,air_temperature&dew_point&relative_humidity&wind_speed&total_precipitable_water&ghi&dni&dhi,2020,true,true,60,Jeon,jjmin416@snu.ac.kr

- wkt = well-known text format: indicating geographical data of data site.
ex) POINT(127.117 35.841) 		(A single blank is needed between the lon and lat value)
- This will be automatically formatted inside of the program, based on the given input data.


- Currently, each line of input data is needed for each http request. (for n times of request, there should be n lines of data inputs)
- The chain of attributes should be connected with '&'
- All possible attribute names can be found in the "Himawari_struct.h" file.

- Data time interval can be 15, 30, 60 (minutes)
- Available years are from 2016 to 2020

- Data name and user email can be set to any arbitrary names.
- Althougth these query info are not used in this program, these are mandatory to utilized the API.



<Himawari_API.cpp>
Read input file -> Make http request -> Asynchronously save response data in memory (in class member linked list)
-> Process saved response data -> Output the data in epw / wth formats.

- Only hourly data (60 minutes) can be requested & processed in current version.
- Daily data will be calculated from the hourly data.
- Only epw (hourly) and wth (processed to daily) output formats are currently avaiable.



****detailed informations (although not organized) can be found in research notes folder.



