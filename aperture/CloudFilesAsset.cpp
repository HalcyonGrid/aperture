#include "stdafx.h"
#include "CloudFilesAsset.h"

namespace aperture {
namespace cloudfiles {

CloudFilesAsset::CloudFilesAsset(const std::string& assetId, boost::shared_ptr<InWorldz::Data::Assets::Stratus::StratusAsset> assetBase)
	: _assetId(assetId), _assetBase(assetBase)
{

}


CloudFilesAsset::~CloudFilesAsset()
{
}

std::string CloudFilesAsset::getUUID() const
{
	return _assetId;
}

size_t CloudFilesAsset::getBinaryDataSize() const
{
	return _assetBase->data().length();
}

aperture::byte CloudFilesAsset::getType() const
{
	return (aperture::byte)_assetBase->type();
}

int CloudFilesAsset::getFullType() const
{
	return _assetBase->type();
}

size_t CloudFilesAsset::copyAssetData(std::string& storage) const
{
	storage = _assetBase->data();
	return _assetBase->data().length();
}

size_t CloudFilesAsset::copyAssetData(std::string& storage, size_t rngStart, size_t rngEnd) const
{
	auto dataSz = _assetBase->data().length();

	if (dataSz == 0) return 0;

	if (rngEnd > (dataSz - 1)) {
		//http spec says clamp it
		rngEnd = dataSz - 1;
	}

	if (rngStart > (dataSz - 1)) {
		//entirely reset the range and just return the whole object
		rngStart = 0;
		rngEnd = dataSz - 1;
	}

	if (rngEnd < rngStart) {
		//entirely reset the range. this seems to be a bug in the viewer,
		//but for flipped ranges apache just returns the entire object
		rngStart = 0;
		rngEnd = dataSz - 1;
	}

	storage.append(&_assetBase->data()[rngStart], (rngEnd - rngStart) + 1);

	return dataSz;
}

}}