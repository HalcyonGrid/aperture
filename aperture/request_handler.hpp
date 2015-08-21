//
// request_handler.hpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_REQUEST_HANDLER_HPP
#define HTTP_REQUEST_HANDLER_HPP

#include <string>
#include <boost/noncopyable.hpp>
#include <boost/function.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/shared_ptr.hpp>

#include <map>
#include <set>
#include <vector>
#include <queue>


#include "IAssetServer.h"
#include "lru_cache.h"
#include "AssetSizeCalculator.h"
#include "IAsset.h"
#include "request.hpp"
#include "reply.hpp"

class TokenBucket;

namespace http {
	namespace server {

		struct reply;
		struct request;

		typedef std::map<std::string, std::string> QueryString;

		struct PackedRequestInfo
		{
			std::string AssetId;
			boost::function<void()> CompletionCallback;
			reply* Reply;
			std::map<std::string, std::string> QueryString;
			request Request;
			bool ServedFromCache;
			bool ServedFromWhip;
			bool ServedFromCF;
		};

		/// The common handler for all incoming requests.
		class request_handler
			: private boost::noncopyable
		{
		public:
			/// Construct with a directory containing files to be served.
			explicit request_handler(aperture::IAssetServer::ptr whipAssetServer, aperture::IAssetServer::ptr cfConnector,
				const std::string& capsToken);

			/// Handle a request and produce a reply.
			void handle_request(const request& req, reply& rep, boost::function<void()> completionCallback);

			/// Handle a request for an asset
			void handle_asset_request(const request& req, reply& rep, 
				boost::function<void()> completionCallback,
				std::vector<std::string>& urlParts);
			
			/// Handle a request for add cap
			void handle_add_cap(const request& req, reply& rep, 
				boost::function<void()> completionCallback,
				std::vector<std::string>& urlParts);

			/// Handle a request for rem cap
			void handle_rem_cap(const request& req, reply& rep, 
				boost::function<void()> completionCallback,
				std::vector<std::string>& urlParts);

			/// Handle a request for pause cap
			void handle_pause_cap(const request& req, reply& rep, 
				boost::function<void()> completionCallback,
				std::vector<std::string>& urlParts);

			/// Handle a request for resume cap
			void handle_resume_cap(const request& req, reply& rep, 
				boost::function<void()> completionCallback,
				std::vector<std::string>& urlParts);

			/// Handle a request to limit the outbound bandwidth from a cap
			void handle_limit_cap(const request& req, reply& rep, 
				boost::function<void()> completionCallback,
				std::vector<std::string>& urlParts);

			/// Configure and enable the asset cache for requests
			void initAssetCache(unsigned int maxSize);

			boost::shared_ptr<TokenBucket> getBucket(const std::string& caps);

		private:
			/// The asset server to request assets from
			aperture::IAssetServer::ptr _whipAssetServer;

			/// A cloudfiles asset server
			aperture::IAssetServer::ptr _cfConnector;

			/// Token requred to manage our caps
			std::string _capsToken;

			/// Holds valid cap UUIDs
			std::set<std::string> _validCapIds;

			/// Holds queued requests and signals that the cap id is paused
			std::map<std::string, std::queue<PackedRequestInfo> > _queuedRequests;

			/// Holds token buckets for the given caps
			std::map<std::string, boost::shared_ptr<TokenBucket> > _capsBuckets; 

			/// Debugging?
			bool _debug;

			/// using a cache?
			bool _useCache;

			/// the cache
			boost::shared_ptr<LRUCache<std::string, aperture::IAsset::ptr, AssetSizeCalculator> > _assetCache;


			/// Perform URL-decoding on a string. Returns false if the encoding was
			/// invalid.
			static bool url_decode(const std::string& in, std::string& out);

			/// Decodes the query string into parts
			static QueryString decode_query_string(const std::string& url);

			/// Called when we have an asset for the reply
			void asset_response_callback(PackedRequestInfo reqInfo, aperture::IAsset::ptr asset);

			boost::tuple<unsigned int, unsigned int> parse_range(const std::string& rangeStr);

			void sendResponse(PackedRequestInfo& reqInfo, aperture::IAsset::ptr asset, const std::string& contentType);

			void processRequest(PackedRequestInfo& reqInfo);
		};

	} // namespace server
} // namespace http

#endif // HTTP_REQUEST_HANDLER_HPP
