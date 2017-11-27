#include "StdAfx.h"

#include "Settings.h"

#include <fstream>
//compiler complains about std::copy inside of boost
#pragma warning(disable:4996)
#include <boost/algorithm/string.hpp>
#pragma warning(default:4996)

namespace po = boost::program_options;
using namespace std;

namespace aperture
{
	const char* const Settings::SETTINGS_FILE = "aperture.cfg";
	Settings* Settings::_instance = 0;

	Settings::Settings(const std::string& configFileName)
		: _configFileName(configFileName)
	{
		this->reload();
	}

	Settings::~Settings()
	{
	}

	void Settings::reload()
	{
		//setup options
		po::options_description desc("Allowed options");
		desc.add_options()
			("http_listen_port", po::value<unsigned short>()->default_value(Settings::DEFAULT_HTTP_PORT), "TCP port to listen for HTTP client connections")
			("enable_whip", po::value<bool>()->default_value(true), "Whether to enable WHIP server connections")
			("whip_url", po::value<std::string>(), "Whip host URL to connect to")
			("debug", po::value<bool>()->default_value(false), "Is debugging enabled")
			("caps_token", po::value<std::string>(), "Token to allow caps addition")
			("cache_size", po::value<unsigned int>()->default_value(0), "Maximum size of the asset cache in bytes")
			("enable_cloudfiles", po::value<bool>()->default_value(false), "Whether to enable cloud files connections")
			("cf_username", po::value<std::string>(), "CloudFiles user name")
			("cf_api_key", po::value<std::string>(), "CloudFiles API key")
			("cf_container_prefix", po::value<std::string>(), "The prefix of the containers we would like to read from CF")
			("cf_region_name", po::value<std::string>(), "The cloudfiles datacenter/region to use")
			("cf_use_internal_url", po::value<bool>()->default_value(false), "Whether or not to use the servicenet URL for CF")
			("cf_worker_threads", po::value<unsigned int>()->default_value(16), "Default number of worker threads to make requests on CF")
		;

		

		//try to load the config file
		std::ifstream configFile(_configFileName.c_str());

		if (configFile.good()) {
			_vm.reset(new po::variables_map());
			po::store(po::parse_config_file(configFile, desc), *_vm);

			this->verifyOptions();

		} else {
			throw std::runtime_error("Could not open configuration file '" + _configFileName + "'");

		}

		po::notify(*_vm);
	}

	void Settings::verifyOptions()
	{
		auto config = *_vm;

		if (config.count("caps_token") == 0)
			throw std::runtime_error("The caps_token option must be specified");

		if (config["enable_whip"].as<bool>()) {
			//make sure we have a whip URL
			if (config.count("whip_url") == 0)
				throw std::runtime_error("When using a WHIP server you must specify whip_url");
		}

		if (config["enable_cloudfiles"].as<bool>()) {
			//cf requires a bunch of stuff to be set
			if (config.count("cf_username") == 0) this->cfConfigError();
			if (config.count("cf_api_key") == 0) this->cfConfigError();
			if (config.count("cf_region_name") == 0) this->cfConfigError();
			if (config.count("cf_container_prefix") == 0) this->cfConfigError();
		}

	}

	void Settings::cfConfigError() const
	{
		throw std::runtime_error("When enable_cloudfiles is set, you must also specify cf_username, cf_api_key, and cf_container_prefix");
	}

	const po::variables_map& Settings::config() const
	{
		return *_vm;
	}

	whip::WhipURI Settings::getWhipURL() const
	{
		whip::WhipURI uri((*_vm)["whip_url"].as<std::string>());
		return uri;
	}
}