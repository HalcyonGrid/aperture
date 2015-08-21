#include "stdafx.h"
#include "CloudFilesConnector.h"

#include "curl/curl.h"
#include "RackspaceAuthorizer.h"
#include "Settings.h"
#include "AppLog.h"

namespace aperture { 
namespace cloudfiles {

CloudFilesConnector::CloudFilesConnector(boost::asio::io_service& ioService)
	: _ioService(ioService)
{
	_stop = false;

	curl_global_init(CURL_GLOBAL_ALL);

	auto config = Settings::instance().config();

	_rsAuth.reset(
		new RackspaceAuthorizer(
			config["cf_username"].as<std::string>(),
			config["cf_api_key"].as<std::string>(),
			config["cf_region_name"].as<std::string>(),
			config["cf_use_internal_url"].as<bool>()
		)
	);

	//log in and get a token
	_rsAuth->requestNewAuthToken();

	AppLog::instance().out() << "[CLOUDFILES] Auth successful, using " << _rsAuth->getCloudFilesUrl() << std::endl;
	//fire up our workers
	unsigned int numWorkers = config["cf_worker_threads"].as<unsigned int>();
	for (unsigned int i = 0; i < numWorkers; i++) {
		CloudFilesGetWorker::ptr worker(new CloudFilesGetWorker(*this, _ioService));
		worker->start();
		_workers.push_back(worker);
	}
}


CloudFilesConnector::~CloudFilesConnector()
{
	for (CloudFilesGetWorker::ptr worker : _workers)
	{
		worker->waitForStop();
	}

	curl_global_cleanup();
}

bool CloudFilesConnector::isConnected() const
{
	return true;
}

void CloudFilesConnector::getAsset(const std::string& uuid, boost::function<void (IAsset::ptr)> callBack)
{
	boost::mutex::scoped_lock lock(_queueMutex);
	_workQueue.push(std::tie(uuid, callBack));
	_signal.notify_one();
}

void CloudFilesConnector::shutdown()
{
}

std::tuple<std::string, boost::function<void (IAsset::ptr)>> CloudFilesConnector::waitForWork()
{
	boost::mutex::scoped_lock lock(_queueMutex);
	while (_workQueue.empty() && !_stop) {
		_signal.wait(lock);
	}

	if (_stop) return std::tuple<std::string, boost::function<void (IAsset::ptr)>>();

	auto work = _workQueue.front();
	_workQueue.pop();

	return work;
}

bool CloudFilesConnector::isStopping() const
{
	return _stop;
}

boost::shared_ptr<RackspaceAuthorizer> CloudFilesConnector::getAuthorizer()
{
	return _rsAuth;
}

}}