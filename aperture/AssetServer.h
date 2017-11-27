#pragma once

#include "AuthChallengeMsg.h"
#include "AuthResponseMsg.h"
#include "AuthStatusMsg.h"
#include "Asset.h"
#include "ClientRequestMsg.h"
#include "ServerResponseMsg.h"
#include "WhipURI.h"
#include "IAsset.h"
#include "IAssetServer.h"

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/function.hpp>

#include <queue>
#include <map>
#include <vector>

namespace whip
{
	class AssetSearch;
	typedef boost::weak_ptr<AssetSearch> AssetSearchWPtr;
	typedef boost::shared_ptr<AssetSearch> AssetSearchPtr;

	/**
	 * Represents a peer server on the mesh network that this server knows about
	 * due to getting a heartbeat signal
	 */
	class AssetServer : public boost::enable_shared_from_this<AssetServer>, public aperture::IAssetServer
	{
	public:
		typedef boost::shared_ptr<AssetServer> ptr;
		typedef boost::weak_ptr<AssetServer> wptr;

		typedef boost::function<void (AssetServer::ptr, bool)> ConnectCallback;
		typedef boost::function<void (AssetServer::ptr)> SafeKillCallback;

		enum ConnectionState 
		{
			CSTATE_DISCONNECTED,
			CSTATE_CONNECTING,
			CSTATE_CONNECTED,
			CSTATE_SHUTDOWN,
			CSTATE_RECONNECT_WAIT
		};

	private:
		static const int RECONNECT_DELAY = 5;

		/**
		 * The URI that identifies this server
		 */
		WhipURI _serverURI;

		/**
		 * The port that the tcp/ip asset service is running on
		 */
		boost::asio::ip::tcp::endpoint _assetServiceEndpoint;

		/**
		 * IO service that will service all requests for this server
		 */
		boost::asio::io_service& _ioService;

		/**
		 * Tcp/Ip socket for accessing asset services on the remote
		 */
		boost::asio::ip::tcp::socket _serviceSocket;

		/**
		 * Whether or not a successful connection was made to the asset service port
		 */
		ConnectionState _connectionState;

		/**
		 * Callback to make when all searches are completed
		 */
		SafeKillCallback _safeKillCallback;

		/**
		 * Stores all pending transfer send requests
		 */
		typedef std::queue<std::string> PendingSendQueue;
		PendingSendQueue _pendingSends;

		/**
		 * Stores all pending transfer receives
		 */
		typedef std::vector<boost::function<void (Asset::ptr)> > AssetCallbackList;
		typedef std::map<std::string, AssetCallbackList> PendingTransferMap;
		PendingTransferMap _pendingTransfers;

		boost::asio::deadline_timer _reconnectTimer;



		void onConnect(const boost::system::error_code& error);
		
		void onRecvChallenge(const boost::system::error_code& error, size_t bytesRcvd, AuthChallengeMsg::ptr challenge);
		
		void onChallengeResponseWrite(const boost::system::error_code& error, size_t bytesSent,
			AuthResponseMsg::ptr authResponse);

		void onAuthenticationStatus(const boost::system::error_code& error, size_t bytesRcvd,
			AuthStatusMsg::ptr authStatus);

		void sendChallengeResponse(AuthChallengeMsg::ptr challenge);


		/**
		 * Asset request process methods
		 */
		void processNextSendItem();

		void tryProcessNextSendItem();

		void onWriteAssetRequest(const boost::system::error_code& error, size_t bytesSent,
			ClientRequestMsg::ptr request);

		void beginResponseHeaderRead();

		void onReadResponseHeader(const boost::system::error_code& error, size_t bytesSent,
			ServerResponseMsg::ptr response);

		void fireAssetRcvdCallbacks(const std::string& assetUUID, Asset::ptr asset);

		void testContinueRecv();

		void onHandleRequestErrorData(const boost::system::error_code& error, size_t bytesSent,
			ServerResponseMsg::ptr response);

		void onReadResponseData(const boost::system::error_code& error, size_t bytesSent,
			ServerResponseMsg::ptr response);

		void tryReconnect();

		void onReconnect(const boost::system::error_code& error);


	public:
		AssetServer(const WhipURI& serverURI, boost::asio::io_service& ioService);
		virtual ~AssetServer();

		/**
		 * Closes all service sockets
		 */
		void close(bool doShutdown = false);

		/**
		 * Connects to the mesh server's asset service
		 */
		void connect();

		/**
		 * Returns the status of the connection to this server
		 */
		ConnectionState getServiceConnectionState() const;

		/**
		 * Returns a simple true or false for whether the server is ready for requests
		 */
		virtual bool isConnected() const;

		/**
		 * Retrieves the given asset from this server
		 */
		virtual void AssetServer::getAsset(const std::string& uuid, boost::function<void (aperture::IAsset::ptr)> callBack);

		/**
		 * Shuts down the connections and marks this server as shut down
		 */
		virtual void shutdown();
	};
}