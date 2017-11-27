#pragma once

#include "byte.h"

namespace aperture {

/**
 * Interface to the important data contained in an asset
 */
class IAsset
{
public:
	typedef boost::shared_ptr<IAsset> ptr;

public:
	virtual ~IAsset();

	/**
	 * Returns the asset UUID
	 */
	virtual std::string getUUID() const = 0;

	/**
	 * Returns the size of the binary asset data
	 */
	virtual unsigned int getBinaryDataSize() const = 0;

	/**
     * Returns the type of asset this is
	 */
	virtual aperture::byte getType() const = 0;

	/**
	 * Copies the asset data only to the given storage area
	 */
	virtual unsigned int copyAssetData(std::string& storage) const = 0;

	/**
	 * Copies the asset data only to the given storage area (rngStart + rngEnd are INCLUSIVE)
	 */
	virtual unsigned int copyAssetData(std::string& storage, unsigned int rngStart, unsigned int rngEnd) const = 0;
};

}