#pragma once

#include "byte.h"
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

namespace whip
{
/**
 * The server's response to a cllient request
 */
class ServerResponseMsg
{
private:
	aperture::byte_array_ptr _data;
	aperture::byte_array _header;

	static const short DATA_LEN_MARKER_LOC = 33;
	static const short RESPONSE_CODE_MARKER_LOC = 0;
	static const short UUID_LOC = 1;

public:
	typedef boost::shared_ptr<ServerResponseMsg> ptr;

	static const short HEADER_SIZE = 37;
	
	enum ResponseCode {
		RC_FOUND = 10,
		RC_NOTFOUND = 11,
		RC_ERROR = 12,
		RC_OK = 13
	};

private:
	void setupHeader(ResponseCode code, const std::string& assetUUID, unsigned int dataLen);

public:
	/**
	 * Constructs a new response message for pooling
	 */
	ServerResponseMsg();

	/**
	 *Constructs a new response message with the given code and no data
	 */
	ServerResponseMsg(ResponseCode code, const std::string& assetUUID);

	/**
	 * Constructs a new response message with the given code and data
	 */
	ServerResponseMsg(ResponseCode code, const std::string& assetUUID, aperture::byte_array_ptr data);

	/**
	 *Constructs a new response message with the given code and string for data
	 */
	ServerResponseMsg(ResponseCode code, const std::string& assetUUID, const std::string& message);


	/**
	DTOR
	*/
	virtual ~ServerResponseMsg();


	/**
	Returns the data array that holds the header
	*/
	aperture::byte_array& getHeader();

	/**
	Returns the array that holds the data
	*/
	aperture::byte_array_ptr getData();

	/**
	 * Allocates the memory for inbound asset data based on the data in the header
	 */
	void initializeDataStorage();

	/**
	 * Validates the header for this message
	 */
	bool validateHeader();

	/**
	 * Returns the response code
	 */
	ResponseCode getResponseCode() const;

	/**
	 * Returns the size of the incoming data according to this header
	 */
	unsigned int getDataSize() const;

	/**
	 * Returns the asset UUID that this response is for
	 */
	std::string getAssetUUID() const;
};

class ServerResponseMsgCreator
{
public: 

	ServerResponseMsg::ptr operator()()
	{
		return boost::make_shared<ServerResponseMsg>();
	}
};
}
