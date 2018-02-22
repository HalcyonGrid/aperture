#include "stdafx.h"

#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>

#include "server.hpp"
#include "AppLog.h"
#include "Settings.h"
#include "AssetServer.h"
#include "WhipURI.h"
#include "CloudFilesConnector.h"
#include "Version.h"

#if defined(_WIN32)

boost::function0<void> console_ctrl_function;

BOOL WINAPI console_ctrl_handler(DWORD ctrl_type)
{
	switch (ctrl_type)
	{
	case CTRL_C_EVENT:
	case CTRL_BREAK_EVENT:
	case CTRL_CLOSE_EVENT:
	case CTRL_SHUTDOWN_EVENT:
		aperture::AppLog::instance().out() << "Clean shutdown initiated" << std::endl;
		console_ctrl_function();
		return TRUE;
	default:
		return FALSE;
	}
}

int main(int argc, char* argv[])
{
	using namespace aperture;

	try
	{
		boost::asio::io_service ioService;

		AppLog::SetLogStream(&std::cout);

		AppLog::instance().out() << "InWorldz Aperture Server " << APERTURE_VERSION << std::endl;
		AppLog::instance().out() << "Build: " << __DATE__ " " __TIME__ << std::endl;

		auto config = Settings::instance().config();
		if (config["debug"].as<bool>()) {
			AppLog::instance().out() << "DEBUGGING ENABLED" << std::endl;
		}

		// Start connection to whip server
		whip::AssetServer::ptr assetServer;
		if (config["enable_whip"].as<bool>()) {
			whip::WhipURI uri = Settings::instance().getWhipURL();
			AppLog::instance().out() << "Starting connection to WHIP server: " << uri.getHostName() << ":" << uri.getPort() << std::endl;
			assetServer.reset(new whip::AssetServer(uri, ioService));
			assetServer->connect();
		}

		// Start connection to cloudfiles
		cloudfiles::CloudFilesConnector::ptr cfConnector;
		if (config["enable_cloudfiles"].as<bool>()) {
			cfConnector.reset(new cloudfiles::CloudFilesConnector(ioService));
		}

		// Initialize server
		std::string capsToken = config["caps_token"].as<std::string>();
		http::server::server s(config["http_listen_port"].as<unsigned short>(), 
			ioService, assetServer, cfConnector, capsToken);

		// Set console control handler to allow server to be stopped.
		console_ctrl_function = boost::bind(&http::server::server::stop, &s);
		SetConsoleCtrlHandler(console_ctrl_handler, TRUE);

		// Run the server until stopped.
		s.run();
	}
	catch (std::exception& e)
	{
		AppLog::instance().out() << "EXCEPTION: " << e.what() << std::endl;
		AppLog::instance().out() << "Application is terminating" << std::endl;
	}

	return 0;
}

#endif // defined(_WIN32)
