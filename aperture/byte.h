#pragma once

#include <vector>
#include <boost/shared_ptr.hpp>

namespace aperture 
{
	typedef unsigned char byte;
	typedef std::vector<aperture::byte> byte_array;
	typedef boost::shared_ptr<byte_array> byte_array_ptr;
}