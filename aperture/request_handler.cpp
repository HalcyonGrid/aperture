//
// request_handler.cpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "stdafx.h"


#include "request_handler.hpp"
#include <fstream>
#include <sstream>
#include <string>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>    
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <limits>
#include <algorithm>

#include "Validator.h"
#include "Settings.h"
#include "AppLog.h"
#include "header.hpp"
#include "TokenBucket.h"
#include "CloudFilesAsset.h"
#include <boost/make_shared.hpp>

using namespace aperture;

namespace http {
	namespace server {
		
		request_handler::request_handler(aperture::IAssetServer::ptr whipAssetServer,
			aperture::IAssetServer::ptr cfConnector, const std::string& capsToken)
			: _whipAssetServer(whipAssetServer), _cfConnector(cfConnector), _capsToken(capsToken), _useCache(false)
		{
			_debug = (aperture::Settings::instance().config())["debug"].as<bool>();
		}

		QueryString request_handler::decode_query_string(const std::string& url)
		{
			enum DecodeState
			{
				DS_BEGIN,
				DS_IN_PARAM_NAME,
				DS_IN_PARAM_VAL,
				DS_INVALID
			};

			QueryString retQuery;
			DecodeState state = DS_BEGIN;
			std::string currParamName;
			std::string currParamVal;
			for (std::size_t i = 0; i < url.size(); ++i)
			{
				if (url[i] == '?')
				{
					state = DS_IN_PARAM_NAME;
					continue;
				}

				if (url[i] == '&' || url[i] == ';')
				{
					if (state == DS_BEGIN || state == DS_IN_PARAM_NAME)
					{
						//invalid
						state = DS_INVALID;
						break;
					}

					//otherwise we should have a completed param
					std::string name;
					std::string val;

					if (! url_decode(currParamName, name))
					{
						state = DS_INVALID;
						break;
					}

					if (! url_decode(currParamVal, val))
					{
						state = DS_INVALID;
						break;
					}

					boost::to_lower(name);
					retQuery[name] = val;
					currParamName.clear();
					currParamVal.clear();

					state = DS_BEGIN;
					continue;
				}

				if (url[i] == '=')
				{
					if (state == DS_BEGIN || state == DS_IN_PARAM_VAL)
					{
						//invalid
						state = DS_INVALID;
						break;
					}

					state = DS_IN_PARAM_VAL;
					continue;
				}
				
				if (state == DS_IN_PARAM_NAME)
				{
					currParamName += url[i];
					continue;
				}

				if (state == DS_IN_PARAM_VAL)
				{
					currParamVal += url[i];
					continue;
				}
			}

			if (state == DS_IN_PARAM_VAL)
			{
				//one last param
				std::string name;
				std::string val;

				if (! url_decode(currParamName, name))
				{
					return retQuery;
				}

				if (! url_decode(currParamVal, val))
				{
					return retQuery;
				}

				boost::to_lower(name);
				retQuery[name] = val;
			}

			return retQuery;
		}

		void request_handler::handle_asset_request(const request& req, reply& rep, 
			boost::function<void()> completionCallback,
			std::vector<std::string>& urlParts)
		{
			if (urlParts.size() < 5) 
			{
				AppLog::instance().out() 
					<< "[HTTP] Bad request for asset, not enough URL parts"
					<< std::endl;

				rep = reply::stock_reply(reply::bad_request);
				completionCallback();
				return;
			}

			// grab the query string parts
			QueryString queryString = decode_query_string(req.uri);

			if (queryString.find("texture_id") == queryString.end() &&
				queryString.find("mesh_id") == queryString.end())
			{
				AppLog::instance().out() 
					<< "[HTTP] Bad request for asset: mesh_id nor texture_id supplied "
					<< std::endl;

				rep = reply::stock_reply(reply::bad_request);
				completionCallback();
				return;
			}

			std::string assetId;
			if (queryString.find("texture_id") != queryString.end()) {
				assetId = queryString["texture_id"];

			} else if (queryString.find("mesh_id") != queryString.end()) {
				assetId = queryString["mesh_id"];

				/*for (auto header : req.headers)
				{
					AppLog::instance().out() 
						<< "[HTTP] Mesh Header: " << header.name << ": " << header.value
						<< std::endl;
				}*/
			}

			boost::replace_all(assetId, "-", "");
			
			//check texture id
			if (! Validator::IsValidUUID(assetId)) 
			{
				AppLog::instance().out() 
					<< "[HTTP] Bad request for asset, invalid UUID supplied"
					<< std::endl;

				rep = reply::stock_reply(reply::bad_request);
				completionCallback();
				return;
			}

			//Check for a valid token
			if (_validCapIds.find(urlParts[3]) == _validCapIds.end())
			{
				rep = reply::stock_reply(reply::not_found);
				completionCallback();
				return;
			}
			
			rep.token = urlParts[3];
			
			PackedRequestInfo reqInfo = {
				assetId, 
				completionCallback,
				&rep,
				queryString,
				req,
				false,
				false,
				false
			};

			//are we paused? if so we need to queue these requests
			if (_queuedRequests.find(urlParts[3]) != _queuedRequests.end())
			{
				const int MAX_QUEUED_REQUESTS = 50;

				std::queue<PackedRequestInfo>& queue = _queuedRequests[urlParts[3]];
				queue.push(reqInfo);

				if (_debug)
				{
					AppLog::instance().out() 
						<< "[HTTP] Queueing asset request since cap " << urlParts[3] << " is paused"
						<< std::endl;
				}

				//if there are too many queued requests, start dumping them
				if (queue.size() > MAX_QUEUED_REQUESTS)
				{
					auto req = queue.front();
					queue.pop();

					*req.Reply = reply::stock_reply(reply::service_unavailable);
					req.CompletionCallback();
				}

				return;
			}

			//nope not paused. process request
			this->processRequest(reqInfo);
		}

		void request_handler::processRequest(PackedRequestInfo& reqInfo)
		{
			//check the cache first.
			if (_useCache) {
				aperture::IAsset::ptr asset = _assetCache->fetch(reqInfo.AssetId);
				if (asset) {
					if (_debug) {
						AppLog::instance().out() 
							<< "[HTTP] Serving asset " << reqInfo.AssetId << " from cache "
							<< std::endl;
					}

					reqInfo.ServedFromCache = true;
					this->asset_response_callback(reqInfo, asset);
					return;
				}
			}
			
			if (_whipAssetServer && _whipAssetServer->isConnected())
			{
				if (_debug)
				{
					AppLog::instance().out() 
						<< "[HTTP] Posting WHIP request for asset " << reqInfo.AssetId
						<< std::endl;
				}

				reqInfo.ServedFromWhip = true;

				_whipAssetServer->getAsset(reqInfo.AssetId, 
					boost::bind(&request_handler::asset_response_callback, this, reqInfo, _1));
			} 
			else if (_cfConnector)
			{
				if (_debug)
				{
					AppLog::instance().out() 
						<< "[HTTP] Posting CF request for asset " << reqInfo.AssetId
						<< std::endl;
				}

				reqInfo.ServedFromCF = true;

				_cfConnector->getAsset(reqInfo.AssetId, 
					boost::bind(&request_handler::asset_response_callback, this, reqInfo, _1));
			}
			else 
			{
				AppLog::instance().out()
					<< "[HTTP] Connection to WHIP server is not established. Sending HTTP 500"
					<< std::endl;

				*reqInfo.Reply = reply::stock_reply(reply::internal_server_error);
				reqInfo.CompletionCallback();
			}
		}

		void request_handler::handle_add_cap(const request& req, reply& rep, 
			boost::function<void()> completionCallback,
			std::vector<std::string>& urlParts)
		{
			if (urlParts.size() < 6)
			{
				AppLog::instance().out() 
					<< "[HTTP][CAPS] Not adding CAP, not enough URL parts"
					<< std::endl;

				rep = reply::stock_reply(reply::bad_request);
				completionCallback();
				return;
			}

			//the 4th part should be the token and should match our settings
			if (urlParts[4] != _capsToken) 
			{
				AppLog::instance().out() 
					<< "[HTTP][CAPS] Not adding CAP, caps token doesnt match"
					<< std::endl;

				rep = reply::stock_reply(reply::bad_request);
				completionCallback();
				return;
			}

			if (_debug)
			{
				AppLog::instance().out() 
					<< "[HTTP][CAPS] Adding CAP " << urlParts[5]
					<< std::endl;
			}

			//the 5th part should be the CAP to add
			_validCapIds.insert(urlParts[5]);

			//the 6th part should be the bandwidth in bytes/sec to use for the cap
			//if it is missing there is no limit
			if (urlParts.size() == 7)
			{
				int bandwidth = boost::lexical_cast<int>(urlParts[6]);

				_capsBuckets[urlParts[5]] = boost::make_shared<TokenBucket>(bandwidth);
			}

			rep = reply::stock_reply(reply::ok);
			completionCallback();
		}

		void request_handler::handle_rem_cap(const request& req, reply& rep, 
			boost::function<void()> completionCallback,
			std::vector<std::string>& urlParts)
		{
			if (urlParts.size() < 6)
			{
				AppLog::instance().out() 
					<< "[HTTP][CAPS] Not removing CAP, not enough URL parts"
					<< std::endl;

				rep = reply::stock_reply(reply::bad_request);
				completionCallback();
				return;
			}

			//the 4th part should be the token and should match our settings
			if (urlParts[4] != _capsToken) 
			{
				AppLog::instance().out() 
					<< "[HTTP][CAPS] Not removing CAP, caps token doesnt match"
					<< std::endl;

				rep = reply::stock_reply(reply::bad_request);
				completionCallback();
				return;
			}

			if (_debug)
			{
				AppLog::instance().out() 
					<< "[HTTP][CAPS] Removing CAP " << urlParts[5]
					<< std::endl;
			}

			//the 5th part should be the CAP to remove
			_validCapIds.erase(urlParts[5]);

			//also remove pending queues
			_queuedRequests.erase(urlParts[5]);

			//and token buckets
			_capsBuckets.erase(urlParts[5]);

			rep = reply::stock_reply(reply::ok);
			completionCallback();
		}

		void request_handler::handle_pause_cap(const request& req, reply& rep, 
			boost::function<void()> completionCallback,
			std::vector<std::string>& urlParts)
		{
			if (urlParts.size() < 6)
			{
				AppLog::instance().out() 
					<< "[HTTP][CAPS] Not pausing CAP, not enough URL parts"
					<< std::endl;

				rep = reply::stock_reply(reply::bad_request);
				completionCallback();
				return;
			}

			//the 4th part should be the token and should match our settings
			if (urlParts[4] != _capsToken) 
			{
				AppLog::instance().out() 
					<< "[HTTP][CAPS] Not pausing CAP, caps token doesnt match"
					<< std::endl;

				rep = reply::stock_reply(reply::bad_request);
				completionCallback();
				return;
			}

			if (_debug)
			{
				AppLog::instance().out() 
					<< "[HTTP][CAPS] Pausing CAP " << urlParts[5]
					<< std::endl;
			}

			//the 5th part should be the CAP to pause
			if (_queuedRequests.find(urlParts[5]) == _queuedRequests.end())
			{
				_queuedRequests[urlParts[5]] = std::queue<PackedRequestInfo>();
			}

			rep = reply::stock_reply(reply::ok);
			completionCallback();
		}

		void request_handler::handle_resume_cap(const request& req, reply& rep, 
			boost::function<void()> completionCallback,
			std::vector<std::string>& urlParts)
		{
			if (urlParts.size() < 6)
			{
				AppLog::instance().out() 
					<< "[HTTP][CAPS] Not removing CAP, not enough URL parts"
					<< std::endl;

				rep = reply::stock_reply(reply::bad_request);
				completionCallback();
				return;
			}

			//the 4th part should be the token and should match our settings
			if (urlParts[4] != _capsToken) 
			{
				AppLog::instance().out() 
					<< "[HTTP][CAPS] Not resuming CAP, caps token doesnt match"
					<< std::endl;

				rep = reply::stock_reply(reply::bad_request);
				completionCallback();
				return;
			}

			if (_debug)
			{
				AppLog::instance().out() 
					<< "[HTTP][CAPS] Resuming CAP " << urlParts[5]
					<< std::endl;
			}

			//the 5th part should be the CAP to resume
			if (_queuedRequests.find(urlParts[5]) != _queuedRequests.end())
			{
				auto queue = _queuedRequests[urlParts[5]];

				//process all queued requests
				while (queue.size() > 0)
				{
					auto reqInfo = queue.front();
					queue.pop();

					if (_debug)
					{
						AppLog::instance().out() 
							<< "[HTTP] Dequeueing asset request for " << urlParts[3] << " unpaused"
							<< std::endl;
					}

					this->processRequest(reqInfo);
				}

				_queuedRequests.erase(urlParts[5]);
			}

			rep = reply::stock_reply(reply::ok);
			completionCallback();
		}

		void request_handler::handle_limit_cap(const request& req, reply& rep, 
			boost::function<void()> completionCallback,
			std::vector<std::string>& urlParts)
		{
			if (urlParts.size() < 7)
			{
				AppLog::instance().out() 
					<< "[HTTP][CAPS] Not limiting CAP, not enough URL parts"
					<< std::endl;

				rep = reply::stock_reply(reply::bad_request);
				completionCallback();
				return;
			}

			//the 4th part should be the token and should match our settings
			if (urlParts[4] != _capsToken) 
			{
				AppLog::instance().out() 
					<< "[HTTP][CAPS] Not limiting CAP, caps token doesnt match"
					<< std::endl;

				rep = reply::stock_reply(reply::bad_request);
				completionCallback();
				return;
			}

			int bwLimit;
			try
			{
				bwLimit = boost::lexical_cast<int>(urlParts[6]);
			}
			catch (const boost::bad_lexical_cast&)
			{
				AppLog::instance().out() 
					<< "[HTTP][CAPS] Not limiting CAP " << urlParts[5] << ". Invalid limit specified."
					<< std::endl;

				rep = reply::stock_reply(reply::bad_request);
				completionCallback();
				return;
			}

            //maximum bandwidth the viewer provides to us with the slider
            const int VIEWER_BW_MAX = 319000;
            if (bwLimit >= VIEWER_BW_MAX)
            {
                const int REPLACEMENT_BW = 625000;

                //if the user is asking for the max bandwidth, set the
                //texture bandwidth to at least 5 mbit instead of 2.5
                bwLimit = std::max(REPLACEMENT_BW, bwLimit);
            }


			if (_debug)
			{
				AppLog::instance().out() 
					<< "[HTTP][CAPS] Setting CAP limit for " << urlParts[5] << " to " << bwLimit
					<< std::endl;
			}

			if (_validCapIds.find(urlParts[5]) == _validCapIds.end())
			{
				AppLog::instance().out() 
					<< "[HTTP][CAPS] Not limiting CAP " << urlParts[5] << ". Cap does not exist."
					<< std::endl;

				rep = reply::stock_reply(reply::bad_request);
				completionCallback();
				return;
			}

			_capsBuckets[urlParts[5]] = boost::make_shared<TokenBucket>(bwLimit);

			rep = reply::stock_reply(reply::ok);
			completionCallback();
		}

		void request_handler::handle_request(const request& req, reply& rep, 
			boost::function<void()> completionCallback)
		{
			//cut up the request by /
			std::vector<std::string> reqParts;
			boost::split(reqParts, req.uri, boost::is_any_of("/"));

			//must have at least 5 parts
			// /CAPS/HTT/ADDCAP/{TOKEN}/{CAP_UUID}
			// /CAPS/HTT/REMCAP/{TOKEN}/{CAP_UUID}
			// /CAPS/HTT/{CAP_UUID}/?texture_id={TEXTUREUUID}

			if (reqParts.size() < 5) 
			{
				rep = reply::stock_reply(reply::bad_request);
				completionCallback();
				return;
			}

			if (reqParts[1] != "CAPS" || reqParts[2] != "HTT")
			{
				rep = reply::stock_reply(reply::bad_request);
				completionCallback();
				return;
			}

			//what type of request is this?
			if (reqParts[3] == "ADDCAP") 
			{
				this->handle_add_cap(req, rep, completionCallback, reqParts);
				
				return;
			}
			else
			if (reqParts[3] == "REMCAP") 
			{
				this->handle_rem_cap(req, rep, completionCallback, reqParts);

				return;
			}
			else
			if (reqParts[3] == "PAUSE")
			{
				this->handle_pause_cap(req, rep, completionCallback, reqParts);

				return;
			}
			else
			if (reqParts[3] == "RESUME")
			{
				this->handle_resume_cap(req, rep, completionCallback, reqParts);

				return;
			}
			else
			if (reqParts[3] == "LIMIT")
			{
				this->handle_limit_cap(req, rep, completionCallback, reqParts);
				return;
			}

			//otherwise we have a texture request
			this->handle_asset_request(req, rep, completionCallback, reqParts);
		}

		bool request_handler::url_decode(const std::string& in, std::string& out)
		{
			out.clear();
			out.reserve(in.size());
			for (std::size_t i = 0; i < in.size(); ++i)
			{
				if (in[i] == '%')
				{
					if (i + 3 <= in.size())
					{
						int value = 0;
						std::istringstream is(in.substr(i + 1, 2));
						if (is >> std::hex >> value)
						{
							out += static_cast<char>(value);
							i += 2;
						}
						else
						{
							return false;
						}
					}
					else
					{
						return false;
					}
				}
				else if (in[i] == '+')
				{
					out += ' ';
				}
				else
				{
					out += in[i];
				}
			}
			return true;
		}

		boost::tuple<unsigned int, unsigned int> request_handler::parse_range(const std::string& rangeStr)
		{
			//the first part of the string should be bytes=X
			//we want to verify this then strip it
			if (rangeStr.substr(0, 6) != "bytes=") {
				throw std::range_error("Invalid range");
			}

			std::string specStr(rangeStr.substr(6));

			std::vector<std::string> strs;
			boost::split(strs, specStr, boost::is_any_of("-"));

/*			if (strs.size() == 1) {
				//starting range only
				return boost::make_tuple(boost::lexical_cast<unsigned int>(strs[0]), std::numeric_limits<unsigned int>::max());
			}*/

			if (strs.size() == 2) {
				//could be just and end or is start an end

				if (strs[0] == "" && strs[1] != "") 
				{
					//just the end
					return boost::make_tuple(0, boost::lexical_cast<unsigned int>(strs[1]));
				}
				else if (strs[0] != "" && strs[1] == "")
				{
#undef max
					//specified start to the end
					return boost::make_tuple(boost::lexical_cast<unsigned int>(strs[0]), std::numeric_limits<unsigned int>::max());
				}
				else if (strs[0] != "" && strs[1] != "")
				{
					//start and end
					return boost::make_tuple(boost::lexical_cast<unsigned int>(strs[0]), 
											 boost::lexical_cast<unsigned int>(strs[1]) );
				}
			}

			throw std::range_error("Invalid range spec: " + specStr);
		}

		void request_handler::asset_response_callback(PackedRequestInfo reqInfo, aperture::IAsset::ptr asset)
		{
			const aperture::byte AT_TEXTURE = 0;
			const aperture::byte AT_MESH = 49;

			if (! asset) {
				if (reqInfo.ServedFromWhip && _cfConnector) {
					//try CF before failing out
					if (_debug) {
						AppLog::instance().out()
							<< "[HTTP] Asset not found on WHIP, trying CF "
							<< reqInfo.AssetId
							<< std::endl;
					}

					reqInfo.ServedFromWhip = false;
					reqInfo.ServedFromCF = true;

					_cfConnector->getAsset(reqInfo.AssetId, 
						boost::bind(&request_handler::asset_response_callback, this, reqInfo, _1));

					return;
				}

				//we have exhausted our options
				if (_debug) {
					AppLog::instance().out()
						<< "[HTTP] Asset not found: "
						<< reqInfo.AssetId
						<< std::endl;
				}

				*reqInfo.Reply = reply::stock_reply(reply::not_found);
				reqInfo.CompletionCallback();
				return;
			}

			if (asset->getType() != AT_TEXTURE && asset->getType() != AT_MESH) {
                std::string source;
                int type = 0;

                if (reqInfo.ServedFromCache) {
                    source = "cache";
                    type = asset->getType();
                }
                if (reqInfo.ServedFromCF) {
                    source = "CF";
                    type = dynamic_cast<cloudfiles::CloudFilesAsset*>(asset.get())->getFullType();
                }
                if (reqInfo.ServedFromWhip) {
                    source = "WHIP";
                    type = asset->getType();
                }

				AppLog::instance().out()
					<< "[HTTP] Not sending non-texture asset " 
                    << reqInfo.AssetId
                    << " from "
                    << source
                    << " to client with asset type: "
                    << type
					<< std::endl;

				*reqInfo.Reply = reply::stock_reply(reply::not_found);
				reqInfo.CompletionCallback();
				return;
			}

			if (! reqInfo.ServedFromCache && _assetCache) {
				//we didnt get this object from the cache, so we should add it
				_assetCache->insert(reqInfo.AssetId, asset);
			}

			size_t errorReportingFullSz = asset->getBinaryDataSize();
			try {
				if (asset->getType() == AT_TEXTURE) {
					this->sendResponse(reqInfo, asset, "image/x-j2c");

				} else {
					this->sendResponse(reqInfo, asset, "application/vnd.ll.mesh");

				}

			} catch (const std::range_error& e) {
				AppLog::instance().out()
					<< "[HTTP] Problem when preparing response to client: "
					<< e.what()
					<< std::endl;
				
				header rangeHeader;
				rangeHeader.name = "Content-Range";
				rangeHeader.value 
					=	std::string("bytes 0") + 
							"-" + 
						boost::lexical_cast<std::string>(errorReportingFullSz - 1) +
							"/" +
						boost::lexical_cast<std::string>(errorReportingFullSz);

				*reqInfo.Reply = reply::stock_reply(reply::range_error,
					rangeHeader);

			} catch (const std::exception& e) {
				AppLog::instance().out()
					<< "[HTTP] Problem when preparing response to client: "
					<< e.what()
					<< std::endl;

				*reqInfo.Reply = reply::stock_reply(reply::bad_request);
			}

			reqInfo.CompletionCallback();
		}

		void request_handler::sendResponse(PackedRequestInfo& reqInfo, IAsset::ptr asset, const std::string& contentType)
		{
			size_t rngBegin, rngEnd = 0;

			bool hasRangeHeader = false;

			for (const header& mheader : reqInfo.Request.headers)
			{
				if (mheader.name == "Range")
				{
					boost::tie(rngBegin, rngEnd) = this->parse_range(mheader.value);
					hasRangeHeader = true;
				}
			}

			size_t fullSz = hasRangeHeader 
			    ? asset->copyAssetData(reqInfo.Reply->content, rngBegin, rngEnd)
			    : asset->copyAssetData(reqInfo.Reply->content);

			if (rngEnd > (fullSz - 1)) rngEnd = (fullSz - 1);

			//all is well..
			reqInfo.Reply->headers.resize(2);
			reqInfo.Reply->headers[0].name = "Content-Length";
			reqInfo.Reply->headers[0].value = boost::lexical_cast<std::string>(reqInfo.Reply->content.size());
			reqInfo.Reply->headers[1].name = "Content-Type";
			reqInfo.Reply->headers[1].value = contentType;

			if (hasRangeHeader && fullSz != reqInfo.Reply->content.size()) {
				reqInfo.Reply->status = reply::partial_content;
				reqInfo.Reply->headers.resize(3);
				reqInfo.Reply->headers[2].name = "Content-Range";
				reqInfo.Reply->headers[2].value 
					=	"bytes " + boost::lexical_cast<std::string>(rngBegin) +
						"-" +
						boost::lexical_cast<std::string>(rngEnd) +
						"/" +
						boost::lexical_cast<std::string>(fullSz);

				if (_debug) {
					AppLog::instance().out()
						<< "[HTTP] Sending range to client "
						<< reqInfo.Reply->headers[2].value 
						<< " for asset "
						<< reqInfo.AssetId
						<< " " << contentType
						<< std::endl;
				}

			} else {
				reqInfo.Reply->status = reply::ok;

				if (_debug) {
					AppLog::instance().out()
						<< "[HTTP] Sending entire asset to client "
						<< reqInfo.AssetId
						<< " " << contentType
						<< std::endl;
				}
			}
		}

		void request_handler::initAssetCache(unsigned int maxSize)
		{
			_assetCache.reset(new LRUCache<std::string, aperture::IAsset::ptr, AssetSizeCalculator>(maxSize));
			_useCache = true;
		}

		boost::shared_ptr<TokenBucket> request_handler::getBucket(const std::string& caps)
		{
			auto iter = _capsBuckets.find(caps);
			if (iter != _capsBuckets.end())
			{
				return iter->second;
			}

			return boost::shared_ptr<TokenBucket>();
		}

	} // namespace server
} // namespace http
