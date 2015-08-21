#include "StdAfx.h"
#include "AssetServer.h"
#include "AppLog.h"
#include "ClientRequestMsg.h"
#include "ServerResponseMsg.h"

#include <boost/bind.hpp>
#include <boost/foreach.hpp>

#include <string>

using namespace boost::posix_time;
using boost::asio::ip::tcp;
using std::string;
using namespace aperture;

namespace whip
{
	AssetServer::AssetServer(const WhipURI& uri, boost::asio::io_service& ioService)
	:	_serverURI(uri),
		_ioService(ioService),
		_serviceSocket(_ioService),
		_connectionState(CSTATE_DISCONNECTED),
		_reconnectTimer(_ioService)
	{
		tcp::resolver resolver(_ioService);
		tcp::resolver::query query(tcp::v4(), uri.getHostName(), boost::lexical_cast<std::string>(uri.getPort()));
		tcp::resolver::iterator addr = resolver.resolve(query);

		_assetServiceEndpoint = *addr;
	}

	AssetServer::~AssetServer()
	{
		try {
			this->close();
		} catch (...) {
		}
	}

	void AssetServer::close(bool doShutdown)
	{
		if (_connectionState != CSTATE_DISCONNECTED && _connectionState != CSTATE_SHUTDOWN) {
			_serviceSocket.close();

			if (doShutdown) _connectionState = CSTATE_SHUTDOWN;

			//if we're shutting down we dont want this close to trigger a reconnect
			if (_connectionState != CSTATE_SHUTDOWN) {
				_connectionState = CSTATE_DISCONNECTED;
			}

			//kill all transfer requests
			
			for (PendingTransferMap::iterator i = _pendingTransfers.begin(); 
				i != _pendingTransfers.end(); ++i)
			{
				AssetCallbackList& callbacks = i->second;
				Asset::ptr nullAsset;
				BOOST_FOREACH(boost::function<void (Asset::ptr)> callback, callbacks)
				{
					callback(nullAsset);
				}
			}

			_pendingTransfers.clear();
			
			PendingSendQueue empty;
			std::swap(_pendingSends, empty);

			if (_connectionState == CSTATE_DISCONNECTED) {
				//begin the reconnect process
				this->tryReconnect();
			}
		}
	}

	void AssetServer::tryReconnect()
	{
		_connectionState = CSTATE_RECONNECT_WAIT;

		_reconnectTimer.expires_from_now(boost::posix_time::seconds(RECONNECT_DELAY));
		_reconnectTimer.async_wait(boost::bind(&AssetServer::onReconnect, this,
            boost::asio::placeholders::error));
	}

	void AssetServer::onReconnect(const boost::system::error_code& error)
	{
		if (! error) {
			this->connect();
		}
	}

	void AssetServer::connect()
	{
		if (_connectionState != CSTATE_SHUTDOWN) {
			_connectionState = CSTATE_CONNECTING;

			_serviceSocket.async_connect(_assetServiceEndpoint,
				boost::bind(&AssetServer::onConnect, this,
				  boost::asio::placeholders::error));
		}
	}

	void AssetServer::shutdown()
	{
		if (_connectionState == CSTATE_RECONNECT_WAIT) {
			_reconnectTimer.cancel();
		}

		this->close(true);
	}

	void AssetServer::onConnect(const boost::system::error_code& error)
	{
		if (! error) {
			//begin authentication, wait for receive challenge
			AuthChallengeMsg::ptr authChallenge(new AuthChallengeMsg(true));

			boost::asio::async_read(_serviceSocket,
				boost::asio::buffer(authChallenge->getDataStorage(), AuthChallengeMsg::MESSAGE_SIZE),
				boost::bind(
				  &AssetServer::onRecvChallenge, this,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred,
					authChallenge));

		} else {
			AppLog::instance().out() 
				<< "[WHIP] Unable to make connection to asset service on intramesh server: "
				<< _assetServiceEndpoint
				<< std::endl;
			
			this->close();
		}
	}

	void AssetServer::onRecvChallenge(const boost::system::error_code& error, size_t bytesRcvd,
		AuthChallengeMsg::ptr challenge)
	{
		if (!error && bytesRcvd != 0) {
			//generate a response to the challenge
			this->sendChallengeResponse(challenge);

		} else {
			AppLog::instance().out() 
				<< "[WHIP] Unable to authenticate with asset service on intramesh server: "
				<< _assetServiceEndpoint
				<< std::endl;
			
			this->close();
		}
	}

	void AssetServer::sendChallengeResponse(AuthChallengeMsg::ptr challenge)
	{
		AuthResponseMsg::ptr respMessage(new AuthResponseMsg(challenge->getPhrase(), 
			_serverURI.getPassword()));

		boost::asio::async_write(_serviceSocket,
			boost::asio::buffer(respMessage->getDataStorage(), AuthResponseMsg::MESSAGE_SIZE),
			boost::bind(&AssetServer::onChallengeResponseWrite, this,
				boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred,
				respMessage));
	}

	void AssetServer::onAuthenticationStatus(const boost::system::error_code& error, size_t bytesRcvd,
			AuthStatusMsg::ptr authStatus)
	{
		if (!error && bytesRcvd > 0) {
			if (! authStatus->validate()) {
				AppLog::instance().out() 
				<< "[WHIP] Unable to authenticate with asset service on intramesh server: "
				<< _assetServiceEndpoint
				<< ": The server's authentication response was invalid"
				<< std::endl;
			
				this->close();
				return;
			}

			if (authStatus->getAuthStatus() != AS_AUTH_SUCCESS) {
				AppLog::instance().out() 
				<< "[WHIP] Unable to authenticate with asset service on intramesh server: "
				<< _assetServiceEndpoint
				<< ": Invalid credentials"
				<< std::endl;
			
				this->close();
				return;
			}

			AppLog::instance().out() 
				<< "[WHIP] Connection established to asset service on server: "
				<< _assetServiceEndpoint
				<< std::endl;

			_connectionState = CSTATE_CONNECTED;

		} else {
			AppLog::instance().out() 
				<< "[WHIP] Unable to authenticate with asset service on intramesh server: "
				<< _assetServiceEndpoint
				<< ": "
				<< error.message()
				<< std::endl;
			
			this->close();
		}
	}

	void AssetServer::onChallengeResponseWrite(const boost::system::error_code& error, size_t bytesSent,
			AuthResponseMsg::ptr response)
	{
		if (!error && bytesSent != 0) {
			AuthStatusMsg::ptr authStatus(new AuthStatusMsg());

			//read the response back from the server
			boost::asio::async_read(_serviceSocket,
			boost::asio::buffer(authStatus->getDataStorage(), AuthStatusMsg::MESSAGE_SIZE),
				boost::bind(&AssetServer::onAuthenticationStatus, this,
					boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred,
					authStatus));

		} else {
			AppLog::instance().out() 
				<< "[WHIP] Unable to authenticate with asset service on intramesh server: "
				<< _assetServiceEndpoint
				<< ": "
				<< error.message()
				<< std::endl;
			
			this->close();
		}
	}

	AssetServer::ConnectionState AssetServer::getServiceConnectionState() const
	{
		return _connectionState;
	}

	bool AssetServer::isConnected() const
	{
		return _connectionState == CSTATE_CONNECTED;
	}

	void AssetServer::getAsset(const std::string& uuid, boost::function<void (aperture::IAsset::ptr)> callBack)
	{
		if (_pendingTransfers.find(uuid) != _pendingTransfers.end()) {
			//we have a transfer in progess for this specific UUID, so the only step taken
			//is to add another tracker for when this asset is available
			//we dont have to send out another request
			_pendingTransfers[uuid].push_back(callBack);

		} else {
			//transfer(s) in progress but nothing for the asset we're looking for
			//send out the request and queue up this asset

			//queue the asset request
			_pendingSends.push(uuid);

			if (_pendingSends.size() == 1) {
				//im the only one in the queue. That means we need to fire this off live
				this->processNextSendItem();
			}

			if (_pendingTransfers.size() == 0) {
				//queue the callback for when this asset comes in
				AssetCallbackList callBacks;
				callBacks.push_back(callBack);
				_pendingTransfers[uuid] = callBacks;

				//just me in here, fire off the recv process
				this->beginResponseHeaderRead();

			} else {
				//queue the callback for when this asset comes in,
				//someone else is already recv and will call me in time
				AssetCallbackList callBacks;
				callBacks.push_back(callBack);
				_pendingTransfers[uuid] = callBacks;

			}

		}
	
	}

	void AssetServer::processNextSendItem()
	{
		ClientRequestMsg::ptr request(new ClientRequestMsg(ClientRequestMsg::RT_GET, _pendingSends.front()));
		boost::asio::async_write(_serviceSocket,
			boost::asio::buffer(request->getHeaderData(), request->getHeaderData().size()),
			boost::bind(&AssetServer::onWriteAssetRequest, this,
				  boost::asio::placeholders::error,
				  boost::asio::placeholders::bytes_transferred,
				  request));
	}

	void AssetServer::tryProcessNextSendItem()
	{
		if (_pendingSends.size() > 0) {
			this->processNextSendItem();
		}
	}

	void AssetServer::onWriteAssetRequest(const boost::system::error_code& error, size_t bytesSent,
		ClientRequestMsg::ptr request)
	{
		if (! error && bytesSent > 0) {
			_pendingSends.pop();
			this->tryProcessNextSendItem();

		} else {
			AppLog::instance().out() 
				<< "[WHIP] Error while reading asset header from mesh server "
				<< _assetServiceEndpoint
				<< ": "
				<< error.message()
				<< std::endl;

			//close the connection something is broken
			this->close();
		}
	}

	void AssetServer::beginResponseHeaderRead()
	{
		//read the response header
		ServerResponseMsg::ptr response(new ServerResponseMsg());
		boost::asio::async_read(_serviceSocket, 
			boost::asio::buffer(response->getHeader(), response->getHeader().size()),
			boost::bind(&AssetServer::onReadResponseHeader, this,
			  boost::asio::placeholders::error,
			  boost::asio::placeholders::bytes_transferred,
			  response));
	}

	void AssetServer::onReadResponseHeader(const boost::system::error_code& error, size_t bytesSent,
		ServerResponseMsg::ptr response)
	{
		if (!error && bytesSent > 0) {
			//is the response valid and positive?
			if (response->validateHeader() && 
				response->getResponseCode() == ServerResponseMsg::RC_FOUND) {
				
				//adjust data size
				response->initializeDataStorage();

				//read the data
#pragma warning (disable: 4503) //decorated name length exceeded
				boost::asio::async_read(_serviceSocket, 
					boost::asio::buffer(*(response->getData()), response->getData()->size()),
					boost::bind(&AssetServer::onReadResponseData, this,
					  boost::asio::placeholders::error,
					  boost::asio::placeholders::bytes_transferred,
					  response));

			} else {
				//error, not found, or other condition
				
				//invalid header disconnect this server
				if (! response->validateHeader()) {
					AppLog::instance().out() 
					<< "[WHIP] Invalid request header sent from server. Terminating connection "
					<< "asset svc ep: " << _assetServiceEndpoint
					<< std::endl;

					this->close();

				} else {
					//if there is data available in the form of an error code we have to read it
					//or it'll muck up our stream positioning
					if (response->getDataSize() > 0) {
						//adjust data size
						response->initializeDataStorage();

						//read the data and discard
						boost::asio::async_read(_serviceSocket, 
							boost::asio::buffer(*(response->getData()), response->getData()->size()),
							boost::bind(&AssetServer::onHandleRequestErrorData, this,
							  boost::asio::placeholders::error,
							  boost::asio::placeholders::bytes_transferred,
							  response));

					} else {
						this->fireAssetRcvdCallbacks(response->getAssetUUID(), Asset::ptr());
						this->testContinueRecv();
					}
				}

				
			}

		} else {
			AppLog::instance().out() 
				<< "[WHIP] Error while reading asset header from mesh server "
				<< _assetServiceEndpoint
				<< ": "
				<< error.message()
				<< std::endl;

			//close the connection something is broken
			this->close();
		}
	}

	void AssetServer::fireAssetRcvdCallbacks(const std::string& assetUUID, Asset::ptr asset)
	{
		PendingTransferMap::iterator i = _pendingTransfers.find(assetUUID);
		if (i != _pendingTransfers.end()) {
			AssetCallbackList& callbacks = i->second;
			BOOST_FOREACH(boost::function<void (Asset::ptr)> callback, callbacks)
			{
				callback(asset);
			}
			
			_pendingTransfers.erase(i);
		}
		
	}
	
	void AssetServer::testContinueRecv()
	{
		if (_pendingTransfers.size() > 0) {
			this->beginResponseHeaderRead();
		}
	}

	void AssetServer::onHandleRequestErrorData(const boost::system::error_code& error, size_t bytesSent,
		ServerResponseMsg::ptr response)
	{
		if (!error && bytesSent > 0) {
			string error;
			error.insert(error.begin(), response->getData()->begin(), response->getData()->end());

			AppLog::instance().out() 
				<< "[WHIP] Error from whip server: "
				<< "asset svc ep: " << _assetServiceEndpoint << ", "
				<< "error msg: " << error
				<< std::endl;

			this->fireAssetRcvdCallbacks(response->getAssetUUID(), Asset::ptr());
			this->testContinueRecv();

		} else {
			this->close();

		}
	}

	void AssetServer::onReadResponseData(const boost::system::error_code& error, size_t bytesSent,
		ServerResponseMsg::ptr response)
	{
		if (!error && bytesSent > 0) {
			//new asset is available
			Asset::ptr meshAsset(new Asset(response->getData()));
			this->fireAssetRcvdCallbacks(response->getAssetUUID(), meshAsset);
			this->testContinueRecv();


		} else {
			AppLog::instance().out() 
				<< "[WHIP] Error while reading asset data from mesh server "
				<< _assetServiceEndpoint
				<< ": "
				<< error.message()
				<< std::endl;

			//close the connection something is broken
			this->close();
		}
	}
}

