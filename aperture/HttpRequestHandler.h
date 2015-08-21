#pragma once

#include <sstream>
#include <string>

namespace aperture {

/**
 * Handles cURL callbacks and extracts the data we want
 */
class HttpRequestHandler
{
private:
	int _lastHttpCode;
	std::stringstream _lastBody;

public:
	HttpRequestHandler();
	virtual ~HttpRequestHandler();

	void reset();

	int getLastHttpCode() const;
	std::stringstream& getLastBody();

	static size_t onHeaderLine(void *ptr, size_t size, size_t nmemb, void *userdata);
	static size_t onData(void *ptr, size_t size, size_t nmemb, void *userdata);

private:
	void handleIncomingHeader(const std::string& header);
	void handleIncomingData(const char* data, size_t size);
};

}