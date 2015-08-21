#pragma once

#include <boost/shared_ptr.hpp>
#include "IAsset.h"

namespace aperture {

/**
 * Any object that can serve as an asset server and retrieve a requested asset
 */
class IAssetServer
{
public:
	typedef boost::shared_ptr<IAssetServer> ptr;

public:
	virtual ~IAssetServer();

	/**
	 * Returns whether or not this server is connected and able to process get requests
	 */
	virtual bool isConnected() const = 0;

	/**
	 * Retrieves the requested asset by ID and calls back the given function with the asset or a null
	 * pointer in the case where the asset could not be found or there was another error
	 */
	virtual void getAsset(const std::string& uuid, boost::function<void (aperture::IAsset::ptr)> callBack) = 0;

	/**
	 * Stops the asset server
	 */
	virtual void shutdown() = 0;
};

}