#pragma once

#include <string>

#include <boost/thread.hpp>
#include <boost/function.hpp>
#include <boost/asio/io_service.hpp>

#include "HttpRequestHandler.h"
#include "IAsset.h"

typedef void CURL;

namespace aperture {
namespace cloudfiles {

class CloudFilesConnector;

/**
 * Does the actual work of calling into cloudfiles via HTTP
 */
class CloudFilesGetWorker
{
public:
	typedef boost::shared_ptr<CloudFilesGetWorker> ptr;

private:
	static const int CONTAINER_UUID_PREFIX_LEN = 4;

	CloudFilesConnector& _parent;
	boost::asio::io_service& _ioService;
	CURL* _curl;
	std::string _containerPrefix;
	bool _debug;
	HttpRequestHandler _reqHandler;

	boost::shared_ptr<boost::thread> _thread;

public:
	CloudFilesGetWorker(CloudFilesConnector& parent, boost::asio::io_service& ioService);
	virtual ~CloudFilesGetWorker();

	void start();
	void waitForStop();

private:
	void run();
	std::string buildContainerUrl(const std::string& assetId) const;
	std::string getContainerName(const std::string& assetId) const;

	void performAssetRequest(const std::string& assetId, boost::function<void (IAsset::ptr)> callback, bool isRetry = false);
};

}}