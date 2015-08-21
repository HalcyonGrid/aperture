#pragma once

#include <boost/shared_ptr.hpp>

#include "IAsset.h"
#include "StratusAsset.pb.h"

namespace aperture {
namespace cloudfiles {

class CloudFilesAsset : public IAsset
{
public:
	typedef boost::shared_ptr<CloudFilesAsset> ptr;

private:
	std::string _assetId;
	boost::shared_ptr<InWorldz::Data::Assets::Stratus::StratusAsset> _assetBase;

public:
	CloudFilesAsset(const std::string& assetId, boost::shared_ptr<InWorldz::Data::Assets::Stratus::StratusAsset> assetBase);
	virtual ~CloudFilesAsset();

	virtual std::string getUUID() const;
	virtual unsigned int getBinaryDataSize() const;
	virtual aperture::byte getType() const;
    virtual int getFullType() const; //for debugging
	virtual unsigned int copyAssetData(std::string& storage) const;
	virtual unsigned int copyAssetData(std::string& storage, unsigned int rngStart, unsigned int rngEnd) const;
};

}}