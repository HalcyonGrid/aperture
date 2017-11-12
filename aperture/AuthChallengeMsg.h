#pragma once

#include "byte.h"

namespace whip
{
/**
 * Message challenging a connector to the whip services
 */
class AuthChallengeMsg
{
private:
	const static short PHRASE_SIZE = 7;
	const static aperture::byte PACKET_IDENTIFIER = 0;
	aperture::byte_array _data;
	
	void generatePhrase();

public:
	const static short MESSAGE_SIZE = 8;
	typedef boost::shared_ptr<AuthChallengeMsg> ptr;

public:
	AuthChallengeMsg(bool inbound);
	virtual ~AuthChallengeMsg();

	/**
	 * Returns the byte array that represents this message on the network
	 */
	const aperture::byte_array& serialize() const;
	
	/**
	 * Returns the byte array that represents this message on the network for writing
	 */
	aperture::byte_array& getDataStorage();
	

	/**
	 * Returns the challenge phrase that was generated
	 */
	std::string getPhrase() const;
};
}