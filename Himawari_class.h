#ifndef HIMAWARI_CLASS_H
#define HIMAWARI_CLASS_H

class Himawari
{
private:
	std::string apiKey;
	std::list<request_param> requestList;
	std::list<response_yearly> responseList;
	std::list<dssat_yearly> dssatList;

public:
	Himawari();
	~Himawari();

	// Getting request parameters.
	void getRequestParam(std::ifstream& reqParam);

	// Sending request & saving response
	void sendRequest();
	static size_t saveResponse(void* dataSaved, size_t unitSize, size_t chunkSize, void* passedPtr);

	void appendQuery(const char* key, std::string value, CURLU* urlHandle);
	void parseResponse(std::string& chunk, std::list<request_param>::iterator iter, std::list<response_yearly>& responseList);

	// Printing response in a file format/
	void epwOutput(std::ofstream& epw);
	void epwCheckNull(double value, std::ofstream& epw, int flag);

	// Get daily data for DSSAT inputs.
	void calcDaily();
	int calcDoy(int year, int month, int day);
	void wthOutput(std::ofstream& wth);
};

#endif