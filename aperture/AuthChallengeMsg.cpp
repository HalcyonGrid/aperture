#include "StdAfx.h"
#include "AuthChallengeMsg.h"

namespace whip
{

AuthChallengeMsg::AuthChallengeMsg(bool inbound)
{
	if (inbound) {
		_data.resize(MESSAGE_SIZE);

	} else {
		throw std::runtime_error("Operation not supported");

	}
}

AuthChallengeMsg::~AuthChallengeMsg()
{
}

void AuthChallengeMsg::generatePhrase()
{
	
}

const aperture::byte_array& AuthChallengeMsg::serialize() const
{
	return _data;
}

aperture::byte_array& AuthChallengeMsg::getDataStorage()
{
	return _data;
}

std::string AuthChallengeMsg::getPhrase() const
{
	return std::string((char*)&_data[1], PHRASE_SIZE);
}
}