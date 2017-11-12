#pragma once

#include "byte.h"

namespace whip
{
/**
 * Enumeration for the auth staus codes
 */
enum AuthStatus
{
	AS_AUTH_SUCCESS = 0,
	AS_AUTH_FAILURE = 1
};

/**
 * The server's response to the client's auth response.  Indicates if the client is
 * allowed to continue processing
 */
class AuthStatusMsg
{
private:
	static const aperture::byte PACKET_IDENTIFIER = 1;

	aperture::byte_array _messageData;

public:
	static const short MESSAGE_SIZE = 2;
	typedef boost::shared_ptr<AuthStatusMsg> ptr;

public:
	/**
	Construct a new auth status message
	*/
	AuthStatusMsg();
	AuthStatusMsg(AuthStatus authStatus);
	virtual ~AuthStatusMsg();

	bool validate() const;

	const aperture::byte_array& serialize() const;
	aperture::byte_array& getDataStorage();

	AuthStatus getAuthStatus() const;
};
}