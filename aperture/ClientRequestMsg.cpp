#include "StdAfx.h"

#include "ClientRequestMsg.h"
#include "Winsock2.h"

ClientRequestMsg::ClientRequestMsg()
: _header(HEADER_SIZE)
{
	
}
ClientRequestMsg::ClientRequestMsg(RequestType type, const std::string& uuid)
{
	_header.push_back((aperture::byte) type);
	_header.insert(_header.end(), uuid.begin(), uuid.end());
	//four zero bytes for the size
	_header.insert(_header.end(), 4, 0);
}


ClientRequestMsg::~ClientRequestMsg()
{
}

void ClientRequestMsg::initDataStorageFromHeader()
{
	//read the size from the header
	unsigned long dataSize = this->getDataSize();
	_data.reset(new aperture::byte_array(dataSize));
}

aperture::byte_array& ClientRequestMsg::getHeaderData()
{
	return _header;
}

ClientRequestMsg::RequestType ClientRequestMsg::getType() const
{
	//make sure we actually have a valid header
	if (_header.size() == 0 || (_header[0] < RT_GET || _header[0] > RT_TEST)) {
		throw std::runtime_error("Bad request, invalid header on ClientRequest");
	}

	return static_cast<ClientRequestMsg::RequestType>(_header[0]);
}

std::string ClientRequestMsg::getUUID() const
{
	return std::string((char*) &_header[1], 32);
}

aperture::byte_array_ptr ClientRequestMsg::getData()
{
	return _data;
}

unsigned int ClientRequestMsg::getDataSize() const
{
	return ntohl(*((unsigned int*)&_header[DATA_SIZE_MARKER_LOC]));
}