#include "stdafx.h"

#include <string>
#include <boost/numeric/conversion/cast.hpp>

#include "AuthResponseMsg.h"
#include "SHA1.h"
#include "Settings.h"

using std::string;
using namespace aperture;

namespace whip
{

/* static */
const aperture::byte AuthResponseMsg::PACKET_IDENTIFIER = 0;
/* static */
const aperture::byte AuthResponseMsg::SERVER_IDENTIFIER = 100;

AuthResponseMsg::AuthResponseMsg()
: _data(AuthResponseMsg::MESSAGE_SIZE)
{

}

AuthResponseMsg::AuthResponseMsg(const std::string& challenge, const std::string& password)
{
	_data.push_back(AuthResponseMsg::PACKET_IDENTIFIER);

	//calculate our response
	string correctHash = this->calculateChallengeResponse(challenge, password);
	std::move(correctHash.begin(), correctHash.end(), std::back_inserter(_data));
}

AuthResponseMsg::~AuthResponseMsg()
{
}

aperture::byte_array& AuthResponseMsg::getDataStorage()
{
	return _data;
}

std::string AuthResponseMsg::getChallengeResponse() const
{
	return string((const char*) &_data[1], AuthResponseMsg::RESPONSE_SIZE);
}

bool AuthResponseMsg::isServerResponse() const
{
	return _data[0] == SERVER_IDENTIFIER;
}

bool AuthResponseMsg::isValid(AuthChallengeMsg::ptr authChallenge)
{
	//check the header
	if (_data[0] != AuthResponseMsg::PACKET_IDENTIFIER &&
		_data[0] != SERVER_IDENTIFIER)
	{
		return false;
	}

	string correctHash = this->calculateChallengeResponse(authChallenge->getPhrase(),
		Settings::instance().config()["password"].as<string>());

	return correctHash == this->getChallengeResponse();

}

std::string AuthResponseMsg::calculateChallengeResponse(const std::string& phrase, const std::string& password)
{
	CSHA1 sha;
	
	//calculate the correct hash based on password and the challenge that was sent
	string correctHash(password + phrase);
	sha.Update((unsigned char*) correctHash.data(), boost::numeric_cast<unsigned int>(correctHash.size()));
	sha.Final();

	sha.ReportHashStl(correctHash, CSHA1::REPORT_HEX_SHORT);

	return correctHash;
}
}
