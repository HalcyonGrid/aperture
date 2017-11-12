#pragma once

#include <queue>
#include <tuple>
#include <vector>

#include <boost/thread.hpp>
#include <boost/function.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/asio/io_service.hpp>

#include "IAsset.h"
#include "IAssetServer.h"
#include "CloudFilesGetWorker.h"

namespace aperture { 
namespace cloudfiles {

class RackspaceAuthorizer;

/**
 * Connector that provides access to assets stored on rackspace cloud files
 */
class CloudFilesConnector : public IAssetServer
{
public:
	typedef boost::shared_ptr<CloudFilesConnector> ptr;

private:
	boost::asio::io_service& _ioService;

	boost::shared_ptr<RackspaceAuthorizer> _rsAuth;
	std::vector<CloudFilesGetWorker::ptr> _workers;
	std::queue<std::tuple<std::string, boost::function<void (IAsset::ptr)>>> _workQueue;

	boost::mutex _queueMutex;
	boost::condition_variable _signal;

	bool _stop;

public:
	CloudFilesConnector(boost::asio::io_service& ioService);
	virtual ~CloudFilesConnector();

	virtual bool isConnected() const;

	virtual void getAsset(const std::string& uuid, boost::function<void (IAsset::ptr)> callBack);

	virtual void shutdown();

	std::tuple<std::string, boost::function<void (IAsset::ptr)>> waitForWork();

	bool isStopping() const;

	boost::shared_ptr<RackspaceAuthorizer> getAuthorizer();
};

}}