//
// request.hpp
// ~~~~~~~~~~~
//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <string>
#include <vector>

#include "header.hpp"

namespace http {
namespace server {

	/// A request received from a client.
	struct request
	{
		typedef boost::shared_ptr<request> ptr;

		std::string method;
		std::string uri;
		int http_version_major;
		int http_version_minor;
		std::vector<header> headers;
		bool is_ready;
		bool is_bad;

		std::string header_value(const std::string& name)
		{
			for (std::vector<header>::iterator i = headers.begin();
				i != headers.end();
				++i)
			{
				if (i->name == name)
				{
					return i->value;
				}
			}

			return "";
		}
	};

} // namespace server
} // namespace http

#endif // HTTP_REQUEST_HPP
