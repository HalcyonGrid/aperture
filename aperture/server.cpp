//
// server.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "stdafx.h"

#include "server.hpp"
#include <boost/bind.hpp>

#include "AppLog.h"
#include "Settings.h"

using namespace aperture;

namespace http {
	namespace server {

		server::server(unsigned short port, boost::asio::io_service& ioService,
			aperture::IAssetServer::ptr whipServer, aperture::IAssetServer::ptr cfConnector,
			const std::string& capsToken)
		: io_service_(ioService),
			acceptor_(io_service_),
			connection_manager_(),
			new_connection_(new connection(io_service_,
				connection_manager_, request_handler_)),
			request_handler_(whipServer, cfConnector, capsToken),
			_whipAssetServer(whipServer),
			_cfConnector(cfConnector)
		{
			unsigned int cacheSize = Settings::instance().config()["cache_size"].as<unsigned int>();

			if (cacheSize != 0) {
				AppLog::instance().out() << "[CACHE] Setting asset cache to " << cacheSize / 1024 / 1024 << " MB" << std::endl;
				request_handler_.initAssetCache(cacheSize);
			} else {
				AppLog::instance().out() << "[CACHE] Asset cache disabled " << std::endl;
			}

			AppLog::instance().out() << "Starting HTTP texture service on TCP/" << port << std::endl;

			// Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
			boost::asio::ip::tcp::endpoint endpoint 
				= boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port);

			acceptor_.open(endpoint.protocol());
			acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
			acceptor_.bind(endpoint);
			acceptor_.listen();
			acceptor_.async_accept(new_connection_->socket(),
				boost::bind(&server::handle_accept, this,
				boost::asio::placeholders::error));
		}

		void server::run()
		{
			// The io_service::run() call will block until all asynchronous operations
			// have finished. While the server is running, there is always at least one
			// asynchronous operation outstanding: the asynchronous accept call waiting
			// for new incoming connections.
			io_service_.run();
		}

		void server::stop()
		{
			// Post a call to the stop function so that server::stop() is safe to call
			// from any thread.
			io_service_.post(boost::bind(&server::handle_stop, this));
		}

		void server::handle_accept(const boost::system::error_code& e)
		{
			if (!e)
			{
				connection_manager_.start(new_connection_);
				new_connection_.reset(new connection(io_service_,
					connection_manager_, request_handler_));
				acceptor_.async_accept(new_connection_->socket(),
					boost::bind(&server::handle_accept, this,
					boost::asio::placeholders::error));
			}
		}

		void server::handle_stop()
		{
			// The server is stopped by cancelling all outstanding asynchronous
			// operations. Once all operations have finished the io_service::run() call
			// will exit.
			acceptor_.close();
			connection_manager_.stop_all();
			if (_whipAssetServer) _whipAssetServer->shutdown();
			if (_cfConnector) _cfConnector->shutdown();
		}

	} // namespace server
} // namespace http
