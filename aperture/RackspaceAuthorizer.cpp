#include "stdafx.h"
#include "RackspaceAuthorizer.h"

#include <exception>
#include <vector>
#include <sstream>

#include <boost/shared_ptr.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>

#include "curl/curl.h"
#include "json_spirit_reader_template.h"

#include "Finally.h"


namespace aperture {
namespace cloudfiles {

	const std::string& RackspaceAuthorizer::AUTH_URL = "https://identity.api.rackspacecloud.com/v2.0/tokens";
	const std::string& RackspaceAuthorizer::LOGIN_STRING = "{\"auth\":{\"RAX-KSKEY:apiKeyCredentials\":{\"username\":\"%1%\", \"apiKey\":\"%2%\"}}}";

	RackspaceAuthorizer::RackspaceAuthorizer(const std::string& userName, const std::string& apiKey,
		const std::string& regionName, bool useInternalUrl)
		: _userName(userName), _apiKey(apiKey), _regionName(regionName), _useInternalUrl(useInternalUrl)
	{
		
	}

	RackspaceAuthorizer::~RackspaceAuthorizer()
	{
		
	}

	const std::string RackspaceAuthorizer::requestNewAuthToken()
	{
		CURL* curl = curl_easy_init();
		if (! curl) throw std::runtime_error(__FILE__ " curl_easy_init() failed"); 

		struct curl_slist *headers=NULL;  
		headers = curl_slist_append(headers, "Content-Type: application/json");

		finally cleanup([&]{ curl_slist_free_all(headers); curl_easy_cleanup(curl); });
		std::string filledLoginString = (boost::format(LOGIN_STRING) % _userName % _apiKey).str();

		curl_easy_setopt(curl, CURLOPT_URL, RackspaceAuthorizer::AUTH_URL.c_str());
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1; .NET CLR 1.1.4322; .NET CLR 2.0.5");
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, filledLoginString.c_str());
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, filledLoginString.length());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30);
		curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, &HttpRequestHandler::onHeaderLine);
		curl_easy_setopt(curl, CURLOPT_WRITEHEADER, &_reqHandler);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &HttpRequestHandler::onData);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &_reqHandler);
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		

		boost::mutex::scoped_lock lock(_lock);
		_reqHandler.reset();
		CURLcode res = curl_easy_perform(curl);

		if(res != CURLE_OK) {
			throw std::runtime_error(std::string(__FILE__ " curl_easy_perform() failed") + curl_easy_strerror(res));
		}

		if (_reqHandler.getLastHttpCode() != 200) {
			throw std::runtime_error( (boost::format(__FILE__ " Authorization failed. HTTP Response: %1%, Body: %2%") % _reqHandler.getLastHttpCode() % _reqHandler.getLastBody().str()).str() );
		}

		json_spirit::mValue value;
		if (! json_spirit::read_stream(_reqHandler.getLastBody(), value)) {
			throw std::runtime_error((boost::format(__FILE__ " Parsing JSON failed. Body: %1%") % _reqHandler.getLastBody().str()).str());
		}

		const json_spirit::mObject& response = value.get_obj();
		auto iaccess = response.find("access");
		if (iaccess == response.end()) {
			throw std::runtime_error((boost::format(__FILE__ " Parsing JSON failed. 'access' node not found. Body: %1%") % _reqHandler.getLastBody().str()).str());
		}

		const json_spirit::mObject& access = iaccess->second.get_obj();

		//do we have a token?
		auto itoken = access.find("token");
		if (itoken == access.end()) {
			throw std::runtime_error((boost::format(__FILE__ " Parsing JSON failed. 'access/token' node not found. Body: %1%") % _reqHandler.getLastBody().str()).str());
		}

		//extract the token information
		auto token = itoken->second.get_obj();
		_authToken = token["id"].get_str();

		auto iserviceCatalog = access.find("serviceCatalog");
		if (iserviceCatalog == access.end()) {
			throw std::runtime_error((boost::format(__FILE__ " Parsing JSON failed. 'access/serviceCatalog' node not found. Body: %1%") % _reqHandler.getLastBody().str()).str());
		}

		//the service catalog is a list
		bool foundEndpoint = false;
		const json_spirit::mArray& serviceCatalog = iserviceCatalog->second.get_array();
		for (auto iObject : serviceCatalog) {

			auto object = iObject.get_obj();
			if (object["name"].get_str() == "cloudFiles") {

				//this node we want. extract the endpoints and find the one we want
				auto endpoints = object["endpoints"].get_array();
				for (auto iendPoint : endpoints) {

					auto endPoint = iendPoint.get_obj();
					if (boost::iequals(endPoint["region"].get_str(), _regionName)) {
						//this is the region we want
						foundEndpoint = true;

						if (_useInternalUrl) {
							_cfUrl = endPoint["internalURL"].get_str();
						} else {
							_cfUrl = endPoint["publicURL"].get_str();
						}
					}
				}
			}
		}

		if (! foundEndpoint) {
			throw std::runtime_error((boost::format(__FILE__ " Target endpoint was not found in response. Body: %1%") % _reqHandler.getLastBody().str()).str());
		}

		return _authToken;
	}

	const std::string RackspaceAuthorizer::getAuthToken()
	{
		boost::mutex::scoped_lock lock(_lock);
		return _authToken;
	}

	const std::string& RackspaceAuthorizer::getCloudFilesUrl() const
	{
		return _cfUrl;
	}
}}