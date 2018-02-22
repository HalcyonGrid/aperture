#include "stdafx.h"

#include "AppLog.h"
#include <boost/date_time/posix_time/posix_time.hpp>

using namespace boost::posix_time;

namespace aperture
{
	AppLog* AppLog::_instance;
	std::ostream* AppLog::_logStream;

	AppLog::AppLog()
	{
	}

	AppLog::~AppLog()
	{
	}


	std::ostream& AppLog::out()
	{
		ptime now(second_clock::local_time());
		(*AppLog::_logStream) << "[" << now << "] ";
		return *AppLog::_logStream;
	}
}
