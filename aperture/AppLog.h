#pragma once
#include <iosfwd>

namespace aperture
{
	/**
	 * Provides simple logging to the application
	 */
	class AppLog
	{
	private:
		static AppLog* _instance;
		static std::ostream* _logStream;

	public:
		/**
		 * Sets the log stream for this logger
		 **/
		static void SetLogStream(std::ostream* logStream) 
		{
			_logStream = logStream;
		}

		/**
		 * Returns the singleton instance
		 **/
		static AppLog& instance() 
		{
			if (! _instance) {
				_instance = new AppLog();
			}

			return *_instance;
		}

		/**
		 * CTOR
		 */
		AppLog();
		virtual ~AppLog();

		/**
		 * Returns the log stream
		 */
		std::ostream& out();
	};

}