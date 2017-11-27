#pragma once

#include <string>
#include <boost/thread/mutex.hpp>

#include "HttpRequestHandler.h"

namespace aperture {
namespace cloudfiles {

/**
 * Used to retrieve an authorization token from rackspace
 */
class RackspaceAuthorizer
{
private:
	static const std::string& AUTH_URL;
	static const std::string& LOGIN_STRING;

	std::string _userName;
	std::string _apiKey;
	std::string _regionName;
	bool _useInternalUrl;
	HttpRequestHandler _reqHandler;

	boost::mutex _lock;

	std::string _authToken;
	std::string _cfUrl;

public:
	RackspaceAuthorizer(const std::string& userName, const std::string& apiKey, 
		const std::string& regionName, bool useInternalUrl);

	virtual ~RackspaceAuthorizer();

	/**
	 * Requests a new authorization token from rackspace
	 */
	const std::string requestNewAuthToken();

	/**
	 * Gets the current auth token we retrieved from rackspace
	 */
	const std::string getAuthToken();

	/**
	 * Returns the cloudfiles URL that was obtained when we retrieved the token
	 */
	const std::string& getCloudFilesUrl() const;
};

}}