#pragma once

#include <string>

namespace whip
{
	/**
	 * A parsed whip URL
	 */
	class WhipURI
	{
	private:
		std::string _hostName;
		unsigned short _port;
		std::string _password;

	public:
		WhipURI(const std::string& uri);
		virtual ~WhipURI();

		const std::string& getHostName() const;
		unsigned short getPort() const;
		const std::string& getPassword() const;
	};
}