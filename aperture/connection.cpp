//
// connection.cpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "stdafx.h"

#include "connection.hpp"

#include <vector>
#include <boost/bind.hpp>

#include "connection_manager.hpp"
#include "request_handler.hpp"
#include "AppLog.h"
#include "Settings.h"
#include "TokenBucket.h"

using namespace boost::posix_time;

namespace http {
	namespace server {

		connection::connection(boost::asio::io_service& io_service,
			connection_manager& manager, request_handler& handler)
			: socket_(io_service),
			connection_manager_(manager),
			request_handler_(handler),
			_closeAfterResponseWritten(false),
			_connKillTimer(io_service),
			_requestInProgress(false),
			_tokenBucketTimer(io_service)
		{
			_debug = (aperture::Settings::instance().config())["debug"].as<bool>();

			if (_debug)
			{
				aperture::AppLog::instance().out() 
					<< "[HTTP] Starting new connection "
					<< std::endl;
			}
		}

		boost::asio::ip::tcp::socket& connection::socket()
		{
			return socket_;
		}

		void connection::init_new_request()
		{
			request::ptr newRequest(new request);
			newRequest->is_bad = false;
			newRequest->is_ready = false;

			requests_.push(newRequest);
		}

		void connection::start()
		{
			_lastActiveTime = second_clock::local_time();
			this->reset_timeout_timer();			
			
			this->init_new_request();
			socket_.async_read_some(boost::asio::buffer(buffer_),
				boost::bind(&connection::handle_read, shared_from_this(),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
		}

		void connection::stop()
		{
			if (_debug)
			{
				aperture::AppLog::instance().out() << "[HTTP] Closing socket" << std::endl;
			}

			socket_.close();
			_connKillTimer.cancel();
			_tokenBucketTimer.cancel();
		}

		void connection::reset_timeout_timer()
		{
			_connKillTimer.expires_from_now(seconds(MAXIMUM_LIVE_TIME));
			
			_connKillTimer.async_wait(
				boost::bind(&connection::on_timeout, shared_from_this(), 
				boost::asio::placeholders::error));
		}

		void connection::process_next_request()
		{
			_lastActiveTime = second_clock::local_time();

			if (requests_.front()->is_bad)
			{
				_requestInProgress = true;

				reply_ = reply::stock_reply(reply::bad_request);
				header connClose;
				connClose.name = "Connection";
				connClose.value = "close";

				reply_.headers.push_back(connClose);

				_closeAfterResponseWritten = true;

				boost::asio::async_write(socket_, reply_.to_buffers(),
					boost::bind(&connection::handle_write, shared_from_this(),
					boost::asio::placeholders::error));
			}
			else if (requests_.front()->is_ready)
			{
				_requestInProgress = true;

				reply_.reset();
				request_handler_.handle_request(*requests_.front(), reply_, 
					boost::bind(&connection::reply_ready, this));
			}
			else
			{
				//nothing is ready, begin a recv
				_requestInProgress = false;

				socket_.async_read_some(boost::asio::buffer(buffer_),
				boost::bind(&connection::handle_read, shared_from_this(),
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
			}
		}

		void connection::handle_read(const boost::system::error_code& e,
			std::size_t bytes_transferred)
		{
			if (!e)
			{
				_lastActiveTime = second_clock::local_time();

				boost::array<char, 8192>::iterator bufferPos = buffer_.data();
				
				while (bufferPos != buffer_.data() + bytes_transferred)
				{
					boost::tribool result;
					boost::tie(result, bufferPos) = request_parser_.parse(
						*requests_.back(), bufferPos, buffer_.data() + bytes_transferred);

					if (result)
					{
						//reset the parser
						request_parser_.reset();
						//push a new starting request to begin building for the next iteration
						this->init_new_request();
					}
					else if (!result)
					{
						//reset the parser
						request_parser_.reset();
						//the connection will be terminated once this request is processed
						break;
					}
					else
					{
						if (! requests_.front()->is_ready)
						{
							//continue receiving until we have at least one completed request
							socket_.async_read_some(boost::asio::buffer(buffer_),
							boost::bind(&connection::handle_read, shared_from_this(),
								boost::asio::placeholders::error,
								boost::asio::placeholders::bytes_transferred));

							break;
						}
					}
				}

				if (requests_.front()->is_ready || requests_.front()->is_bad)
				{
					this->process_next_request();
				}
			}
			else
			{
				if (_debug)
				{
					aperture::AppLog::instance().out() 
						<< "[HTTP] Read error, Stopping connection "
						<< std::endl;
				}

				connection_manager_.stop(shared_from_this());
			}
		}

		void connection::handle_write(const boost::system::error_code& e)
		{
			if (!e)
			{
				if (_closeAfterResponseWritten)
				{
					requests_.pop();
					
					if (_debug)
					{
						aperture::AppLog::instance().out() 
							<< "[HTTP] Closing connection: closeAfterResponseWritten:  "
							<< _closeAfterResponseWritten << std::endl;
					}

					// Initiate graceful connection closure.
					boost::system::error_code ignored_ec;
					socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
					connection_manager_.stop(shared_from_this());
				}
				else
				{
					requests_.pop();

					//handle next request
					this->process_next_request();
				}
			}
			else
			{
				connection_manager_.stop(shared_from_this());
			}
		}

		void connection::reply_ready()
		{
			if (requests_.front()->header_value("Connection") == "close"
				|| _closeAfterResponseWritten)
			{
				header connClose;
				connClose.name = "Connection";
				connClose.value = "close";

				reply_.headers.push_back(connClose);

				_closeAfterResponseWritten = true;
			}

			boost::shared_ptr<TokenBucket> bucket = request_handler_.getBucket(reply_.token);

			if (bucket)
			{
				reply_.token_bucket = bucket;
				boost::asio::async_write(socket_, reply_.header_buffers(),
					boost::bind(&connection::handle_tb_header_write, shared_from_this(),
					boost::asio::placeholders::error));
			}
			else
			{
				boost::asio::async_write(socket_, reply_.to_buffers(),
					boost::bind(&connection::handle_write, shared_from_this(),
					boost::asio::placeholders::error));
			}
		}

		void connection::handle_tb_header_write(const boost::system::error_code& e)
		{
			if (!e)
			{
				this->try_send_next_chunk();
			}
			else
			{
				connection_manager_.stop(shared_from_this());
			}
		}

		void connection::try_send_next_chunk()
		{
			_lastActiveTime = second_clock::local_time();

			if (reply_.bytes_sent == reply_.content.size())
			{
				//done
				this->handle_write(boost::system::error_code());
				return;
			}

			//otherwise, check to see if we have enough tokens to send now
			size_t transmissionSize = std::min(reply_.remaining_bytes(), (size_t)reply_.token_bucket->getMaxBurst());
			if (reply_.token_bucket->removeTokens(transmissionSize))
			{
				//we have enough tokens, send out a chunk
				boost::asio::async_write(socket_, reply_.get_next_chunk(transmissionSize),
						boost::bind(&connection::handle_tb_write, shared_from_this(),
						boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred));
			}
			else
			{
				//we do not have enough tokens, schedule a retry
				_tokenBucketTimer.expires_from_now(std::chrono::milliseconds((int)(TokenBucket::DRIP_PERIOD_PERIOD_MS)));
				_tokenBucketTimer.async_wait(boost::bind(&connection::on_tb_wait_timeout, this, boost::asio::placeholders::error));
			}
		}

		void connection::on_tb_wait_timeout(const boost::system::error_code& e)
		{
			if (e != boost::asio::error::operation_aborted)
			{
				this->try_send_next_chunk();
			}
		}

		void connection::handle_tb_write(const boost::system::error_code& e,
			size_t bytes_transferred)
		{
			if (!e)
			{
				reply_.bytes_sent += bytes_transferred;
				this->try_send_next_chunk();
			}
			else
			{
				this->handle_write(e);
			}
		}

		void connection::on_timeout(const boost::system::error_code& e)
		{
			if (e != boost::asio::error::operation_aborted)
			{
				if (second_clock::local_time() - _lastActiveTime > seconds(MAXIMUM_LIVE_TIME) &&
					! _requestInProgress)
				{
					if (_debug)
					{
						aperture::AppLog::instance().out()
							<< "[HTTP] Timeout waiting for command, disconnecting"
							<< std::endl;
					}

					// Initiate graceful connection closure.
					// this will cause a read error and close out the socket
					boost::system::error_code ignored_ec;
					socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
					socket_.close();
					//connection_manager_.stop(shared_from_this());
				}
				else
				{
					this->reset_timeout_timer();
				}
			}
		}

	} // namespace server
} // namespace http
