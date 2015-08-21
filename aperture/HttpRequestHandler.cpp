#include "stdafx.h"
#include "HttpRequestHandler.h"

#include <boost/algorithm/string.hpp>


namespace aperture {

HttpRequestHandler::HttpRequestHandler()
{
	this->reset();
}

HttpRequestHandler::~HttpRequestHandler()
{
}

void HttpRequestHandler::reset()
{
	_lastHttpCode = 0;
	_lastBody.str("");
	_lastBody.clear();
}

int HttpRequestHandler::getLastHttpCode() const
{
	return _lastHttpCode;
}

std::stringstream& HttpRequestHandler::getLastBody()
{
	return _lastBody;
}


size_t HttpRequestHandler::onHeaderLine(void *ptr, size_t size, size_t nmemb, void *userdata)
{
	HttpRequestHandler* self = static_cast<HttpRequestHandler*>(userdata);

	self->handleIncomingHeader(std::string(static_cast<const char*>(ptr), size * nmemb));
		
	return size * nmemb;
}

void HttpRequestHandler::handleIncomingHeader(const std::string& header)
{
	std::vector<std::string> splitResult;
	boost::split(splitResult, header, boost::is_any_of(" "));

	if (splitResult.size() < 3) {
		return;
	}

	if (boost::iequals(splitResult[0].substr(0,5), "HTTP/")) {
		//the return code should be after HTTP/x.y
		_lastHttpCode = boost::lexical_cast<int>(splitResult[1]);
	}
}

size_t HttpRequestHandler::onData(void *ptr, size_t size, size_t nmemb, void *userdata)
{
	HttpRequestHandler* self = static_cast<HttpRequestHandler*>(userdata);

	self->handleIncomingData(static_cast<const char*>(ptr), size * nmemb);
		
	return size * nmemb;
}

void HttpRequestHandler::handleIncomingData(const char* data, size_t size)
{
	_lastBody.write(data, size);
}

}