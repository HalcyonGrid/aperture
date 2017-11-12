//
// connection.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_CONNECTION_HPP
#define HTTP_CONNECTION_HPP

#include "stdafx.h"

#include <queue>

#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/array.hpp>
#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "reply.hpp"
#include "request.hpp"
#include "request_handler.hpp"
#include "request_parser.hpp"
#include "AssetServer.h"

namespace http {
namespace server {

class connection_manager;

/// Represents a single connection from a client.
class connection
  : public boost::enable_shared_from_this<connection>,
    private boost::noncopyable
{
public:
  /// Construct a connection with the given io_service.
  explicit connection(boost::asio::io_service& io_service,
      connection_manager& manager, request_handler& handler);

  /// Get the socket associated with the connection.
  boost::asio::ip::tcp::socket& socket();

  /// Start the first asynchronous operation for the connection.
  void start();

  /// Stop all asynchronous operations associated with the connection.
  void stop();

private:
  /// Maximum time for this connection to stay open for requests in seconds
  static const int MAXIMUM_LIVE_TIME = 30;

  /// Handle completion of a read operation.
  void handle_read(const boost::system::error_code& e,
      std::size_t bytes_transferred);

  /// Handle completion of a write operation.
  void handle_write(const boost::system::error_code& e);

  /// Handle completion of a header write operation.
  void handle_tb_header_write(const boost::system::error_code& e);

  /// Used with token bucket throttling. Checks the token bucket for tokens
  /// and tries to send the next piece of the response
  void try_send_next_chunk();

   /// Handle completion of a token bucket based write operation.
  void handle_tb_write(const boost::system::error_code& e,
	  size_t bytes_transferred);

  void reply_ready();

  void init_new_request();

  void process_next_request();

  void on_timeout(const boost::system::error_code& e);

  void on_tb_wait_timeout(const boost::system::error_code& e);

  void reset_timeout_timer();

  /// Socket for the connection.
  boost::asio::ip::tcp::socket socket_;

  /// The manager for this connection.
  connection_manager& connection_manager_;

  /// The handler used to process the incoming request.
  request_handler& request_handler_;

  /// Buffer for incoming data.
  boost::array<char, 8192> buffer_;

  /// A list of waiting requests
  std::queue<request::ptr> requests_;

  /// The parser for the incoming request.
  request_parser request_parser_;

  /// The reply to be sent back to the client.
  reply reply_;

  /// Asset server we're working with
  whip::AssetServer::ptr _assetServer;

  /// Time this connection was last active
  boost::posix_time::ptime _lastActiveTime;

  /// Whether or not to close the connection after the response is written
  bool _closeAfterResponseWritten;

  /// If debugging is enabled
  bool _debug;

  /// Closes the connection after MAXIMUM_LIVE_TIME
  boost::asio::deadline_timer _connKillTimer;

  /// Whether or not a request is being processed
  bool _requestInProgress;

  boost::asio::steady_timer _tokenBucketTimer;
};

typedef boost::shared_ptr<connection> connection_ptr;

} // namespace server
} // namespace http

#endif // HTTP_CONNECTION_HPP
