#pragma once

#include <boost/program_options.hpp>
#include <boost/shared_ptr.hpp>
#include <string>

#include "WhipURI.h"

namespace aperture
{
	/**
	 * Singleton class for accessing configuration settings
	 */
	class Settings
	{
	public:
		typedef boost::shared_ptr<boost::program_options::variables_map> VariablesMapPtr;

	private:
		const static short DEFAULT_HTTP_PORT = 8000;

		static const char* const SETTINGS_FILE;
		static Settings* _instance;
		VariablesMapPtr _vm;
		std::string _configFileName;

		void verifyOptions();
		void cfConfigError() const;

	public:
		/**
		Retrieves the single instance of the settings object
		*/
		static Settings& instance() {
			if (! _instance) {
				_instance = new Settings(SETTINGS_FILE);
			}

			return *_instance;
		}

	public:
		Settings(const std::string& settingsFile);
		virtual ~Settings();

		const boost::program_options::variables_map& config() const;

		/**
		 * Reloads the settings from the config file on disk
		 */
		void reload();

		/**
		 * Returns the parsed whip url stored in the whip_url setting
		 */
		whip::WhipURI getWhipURL() const;
	};

}