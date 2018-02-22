#pragma once

#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>

#include "byte.h"
#include "IAsset.h"

namespace whip
{
/**
 * Represents a single asset with data and metadata both in
 * storage and on the wire
 */
class Asset : public aperture::IAsset
{
private:
	static const unsigned int NAME_FIELD_SZ_LOC = 39;

	aperture::byte_array_ptr _data;

public:
	typedef boost::shared_ptr<Asset> ptr;

public:
	/**
		Constructs an asset with a data array pre allocated to the given size
	*/
	explicit Asset(unsigned int sizeHint);

	/**
		Constructs an asset from the given byte array	
	*/
	Asset(aperture::byte_array_ptr data);
	
	virtual ~Asset();

	/**
		Returns the UUID of this asset from the first 32 bytes 
		of packet data
	*/
	std::string getUUID() const override;

	/**
		Returns the size of the data array holding the asset
		data and metadata
	*/
	unsigned int getSize() const;

	/**
		Returns the size of the binary asset data
	*/
	size_t getBinaryDataSize() const override;

	/**
		Returns the internal data for this asset
	*/
	aperture::byte_array_ptr getData() const;

	/**
		Returns whether or not this is a local asset
	*/
	bool isLocal() const;

	/**
		Returns the type of asset this is
	*/
	aperture::byte getType() const override;

	/**
	 * Copies the asset data only to the given storage area
	 */
	size_t copyAssetData(std::string& storage) const override;

	/**
	 * Copies the asset data only to the given storage area (rngStart + rngEnd are INCLUSIVE)
	 */
	size_t copyAssetData(std::string& storage, size_t rngStart, size_t rngEnd) const override;

	/**
	 * Finds the location and size of the actual internal asset binary data
	 */
	boost::tuple<unsigned int, unsigned int> findDataLocationAndSize() const;
};
}