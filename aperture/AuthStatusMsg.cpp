#include "stdafx.h"
#include "AuthStatusMsg.h"

namespace whip
{

/* static */
const aperture::byte AuthStatusMsg::PACKET_IDENTIFIER = 1;

AuthStatusMsg::AuthStatusMsg()
: _messageData(MESSAGE_SIZE)
{
}

AuthStatusMsg::AuthStatusMsg(AuthStatus authStatus)
{
	_messageData.push_back(AuthStatusMsg::PACKET_IDENTIFIER);
	_messageData.push_back(static_cast<aperture::byte>(authStatus));
}

AuthStatusMsg::~AuthStatusMsg()
{
}

const aperture::byte_array& AuthStatusMsg::serialize() const
{
	return _messageData;
}

aperture::byte_array& AuthStatusMsg::getDataStorage()
{
	return _messageData;
}

AuthStatus AuthStatusMsg::getAuthStatus() const
{
	return (AuthStatus) _messageData[1];
}

bool AuthStatusMsg::validate() const
{
	if (_messageData.size() != MESSAGE_SIZE) {
		return false;
	}

	if (_messageData[0] != AuthStatusMsg::PACKET_IDENTIFIER) {
		return false;
	}

	return !(_messageData[1] != AS_AUTH_SUCCESS &&
    		_messageData[1] != AS_AUTH_FAILURE);

}
}
