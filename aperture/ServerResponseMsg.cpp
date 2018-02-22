#include "stdafx.h"
#include "ServerResponseMsg.h"
#include "Winsock2.h"

#include <boost/numeric/conversion/cast.hpp>

namespace whip
{
ServerResponseMsg::ServerResponseMsg()
: _header(HEADER_SIZE)
{
}

ServerResponseMsg::ServerResponseMsg(ResponseCode code, const std::string& assetUUID)
{
	this->setupHeader(code, assetUUID, 0);
}

ServerResponseMsg::ServerResponseMsg(ResponseCode code, const std::string& assetUUID, aperture::byte_array_ptr data)
: _data(data)
{
	this->setupHeader(code, assetUUID, boost::numeric_cast<unsigned int>(_data->size()));
}

ServerResponseMsg::ServerResponseMsg(ResponseCode code, const std::string& assetUUID, const std::string& message)
: _data(new aperture::byte_array(message.begin(), message.end()))
{
	this->setupHeader(code, assetUUID, boost::numeric_cast<unsigned int>(_data->size()));
}

void ServerResponseMsg::setupHeader(ResponseCode code, const std::string& assetUUID, unsigned int dataLen)
{
	_header.reserve(HEADER_SIZE);

	_header.push_back(static_cast<aperture::byte>(code));
	_header.insert(_header.begin() + 1, assetUUID.begin(), assetUUID.end());
	_header.insert(_header.end(), 4, 0);
	*((unsigned int*)(&_header[DATA_LEN_MARKER_LOC])) = htonl(dataLen);
}

ServerResponseMsg::~ServerResponseMsg()
{
}

aperture::byte_array& ServerResponseMsg::getHeader()
{
	return _header;
}

aperture::byte_array_ptr ServerResponseMsg::getData()
{
	return _data;
}

bool ServerResponseMsg::validateHeader()
{
	if (_header.size() != HEADER_SIZE) {
		return false;
	}

	if (this->getResponseCode() < RC_FOUND || this->getResponseCode() > RC_OK) {
		return false;
	}

	return true;
}

void ServerResponseMsg::initializeDataStorage()
{
	unsigned int dataLen = ntohl(*((unsigned int*)(&_header[DATA_LEN_MARKER_LOC])));
	_data.reset(new aperture::byte_array(dataLen));
}

ServerResponseMsg::ResponseCode ServerResponseMsg::getResponseCode() const
{
	return (ResponseCode) _header[RESPONSE_CODE_MARKER_LOC];
}

unsigned int ServerResponseMsg::getDataSize() const
{
	unsigned int dataLen = ntohl(*((unsigned int*)(&_header[DATA_LEN_MARKER_LOC])));
	return dataLen;
}

std::string ServerResponseMsg::getAssetUUID() const
{
	return std::string((const char*)&_header[UUID_LOC], 32);
}

}
