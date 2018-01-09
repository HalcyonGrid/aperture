#include "StdAfx.h"
#include "Asset.h"
#include "Winsock2.h"

#include <boost/numeric/conversion/cast.hpp>
#include <stdexcept>

namespace whip
{
Asset::Asset(unsigned int sizeHint)
: _data(new aperture::byte_array(sizeHint))
{
}

Asset::Asset(aperture::byte_array_ptr data)
: _data(data)
{

}

Asset::~Asset()
{
}

std::string Asset::getUUID() const
{
	return std::string((const char*) &((*_data)[0]), 32);
}

unsigned int Asset::getSize() const
{
	return boost::numeric_cast<unsigned int>(_data->size());
}

aperture::byte_array_ptr Asset::getData() const
{
	return _data;
}

bool Asset::isLocal() const
{
	return (*_data)[33] == 1;
}

aperture::byte Asset::getType() const
{
	return (*_data)[32];
}

size_t Asset::getBinaryDataSize() const
{
	unsigned int loc, size;
	boost::tie(loc, size) = this->findDataLocationAndSize();

	return size;
}

boost::tuple<unsigned int, unsigned int> Asset::findDataLocationAndSize() const
{
	//first variable byte string starts at the name field
	unsigned int currLoc = NAME_FIELD_SZ_LOC;
	aperture::byte nameFieldSz = (*_data)[currLoc];
	//skip those bytes
	currLoc += nameFieldSz + 1;
	
	//next up is the sz of the description field, skip those bytes
	aperture::byte descFieldSz = (*_data)[currLoc];
	currLoc += descFieldSz + 1;

	//finally we're at the data size
	aperture::byte* arrayPart = &((*_data)[currLoc]);
	unsigned int dataSz = ntohl( *((unsigned int*) arrayPart) );

	return boost::tuple<unsigned int, unsigned int>(currLoc + sizeof(unsigned int), dataSz);
}

size_t Asset::copyAssetData(std::string& storage) const 
{
	unsigned int dataLoc;
	unsigned int dataSz;

	boost::tie(dataLoc, dataSz) = this->findDataLocationAndSize();

	if (dataLoc + dataSz > _data->size()) {
		throw std::runtime_error("Asset::copyAssetData(): Copying the full reported data size would overflow buffer");
	}

	storage.append(reinterpret_cast<char*>(&((*_data)[dataLoc])), dataSz);

	return dataSz;
}

size_t Asset::copyAssetData(std::string& storage, size_t rngStart, size_t rngEnd) const
{
	unsigned int dataLoc;
	unsigned int dataSz;

	boost::tie(dataLoc, dataSz) = this->findDataLocationAndSize();

	//first and foremost, if the asset is empty just return here
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

	if (dataLoc + rngEnd >= _data->size()) {
		throw std::range_error("Asset::copyAssetData(): Invalid range sepecification, end of range would overflow buffer");
	}

	char* targetData = reinterpret_cast<char*>( &((*_data)[dataLoc + rngStart]) );
	storage.append(targetData, (rngEnd - rngStart) + 1);

	return dataSz;
}

}