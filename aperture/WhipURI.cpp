#include "StdAfx.h"

#include "WhipURI.h"
#include <vector>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

using namespace std;

namespace whip
{
	WhipURI::WhipURI(const std::string& url)
	{
		string uri(url);
		//parse the uri
		//url must start with whip
		if (uri.substr(0, 7) != "whip://") throw std::runtime_error("Invaid whip URL.  Must start with whip://");

        //strip the Resource ID portion
        uri = uri.substr(7);

        //split by @ this will give us     username:password   ip:port
		vector<string> userAndHost;
		boost::split(userAndHost, uri, boost::is_any_of("@"));
		if (userAndHost.size() != 2) throw std::runtime_error("Invalid whip URL, missing @");

        //get the user and pass
        string pass = userAndHost[0];
       
        //get the host and port
		vector<string> hostAndPort;
		boost::split(hostAndPort, userAndHost[1], boost::is_any_of(":"));
		if (hostAndPort.size() != 2) throw std::runtime_error("Invalid whip URL, missing : between host and port");

        _password = pass;
        _hostName = hostAndPort[0];
		_port = boost::lexical_cast<unsigned short>(hostAndPort[1]);
	}

	WhipURI::~WhipURI()
	{
	}

	const std::string& WhipURI::getHostName() const
	{
		return _hostName;
	}

	unsigned short WhipURI::getPort() const
	{
		return _port;
	}

	const std::string& WhipURI::getPassword() const
	{
		return _password;
	}
}