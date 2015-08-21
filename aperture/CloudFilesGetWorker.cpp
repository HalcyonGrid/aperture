#include "stdafx.h"

#include <tuple>
#include <functional>
#include <sstream>
#include <fstream>

#include <boost/format.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/case_conv.hpp>

#include <google/protobuf/message.h>

#include "curl/curl.h"
#include "Finally.h"

#include "CloudFilesGetWorker.h"
#include "CloudFilesConnector.h"
#include "RackspaceAuthorizer.h"
#include "Settings.h"
#include "AppLog.h"
#include "StratusAsset.pb.h"
#include "CloudFilesAsset.h"

namespace aperture {
namespace cloudfiles {

CloudFilesGetWorker::CloudFilesGetWorker(CloudFilesConnector& parent, boost::asio::io_service& ioService)
	: _parent(parent), _ioService(ioService)
{
	_curl = curl_easy_init();
	if (! _curl) throw std::runtime_error(__FILE__ " curl_easy_init() failed"); 

	_containerPrefix = Settings::instance().config()["cf_container_prefix"].as<std::string>();
	_debug = Settings::instance().config()["debug"].as<bool>();
}


CloudFilesGetWorker::~CloudFilesGetWorker()
{
}

void CloudFilesGetWorker::start()
{
	_thread.reset(new boost::thread(std::bind(&CloudFilesGetWorker::run, this)));
}

void CloudFilesGetWorker::waitForStop()
{
	_thread->join();
}

void CloudFilesGetWorker::run()
{
	while (! _parent.isStopping()) {
		std::tuple<std::string, boost::function<void (IAsset::ptr)>> work = _parent.waitForWork();

		if (std::get<0>(work) == "") {
			//poison pill
			break;
		}

		try
		{
			//otherwise we have a job to do
			this->performAssetRequest(std::get<0>(work), std::get<1>(work));
		}
		catch (const std::exception& e) {
			_ioService.post([=]{ AppLog::instance().out() << "[CLOUDFILES] Caught exception while trying to get asset: " << e.what() << std::endl; });
		}

	}
}

std::string CloudFilesGetWorker::getContainerName(const std::string& assetId) const
{
	return _containerPrefix + boost::to_upper_copy(assetId.substr(0, CONTAINER_UUID_PREFIX_LEN));
}

std::string CloudFilesGetWorker::buildContainerUrl(const std::string& assetId) const
{
	std::stringstream builder;
	builder << _parent.getAuthorizer()->getCloudFilesUrl() << "/" << this->getContainerName(assetId) << "/"
		<< boost::to_lower_copy(boost::algorithm::replace_all_copy(assetId, "-", "")) << ".asset";

	return builder.str();
}

void CloudFilesGetWorker::performAssetRequest(const std::string& assetId, boost::function<void (IAsset::ptr)> callback, bool isRetry)
{
	struct curl_slist *headers=NULL;  
	std::string authHeader((boost::format("X-Auth-Token: %1%") % _parent.getAuthorizer()->getAuthToken()).str());
	headers = curl_slist_append(headers, authHeader.c_str());

	finally cleanup([&]{ curl_slist_free_all(headers); _reqHandler.reset(); });

	std::string url(this->buildContainerUrl(assetId));

	curl_easy_setopt(_curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(_curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(_curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(_curl, CURLOPT_USERAGENT, "Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1; .NET CLR 1.1.4322; .NET CLR 2.0.5");
    curl_easy_setopt(_curl, CURLOPT_POST, 0L);
    curl_easy_setopt(_curl, CURLOPT_TIMEOUT, 30);
	curl_easy_setopt(_curl, CURLOPT_HEADERFUNCTION, &HttpRequestHandler::onHeaderLine);
	curl_easy_setopt(_curl, CURLOPT_WRITEHEADER, &_reqHandler);
	curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, &HttpRequestHandler::onData);
	curl_easy_setopt(_curl, CURLOPT_WRITEDATA, &_reqHandler);
	curl_easy_setopt(_curl, CURLOPT_HTTPHEADER, headers);
	
	if (_debug) {
		_ioService.post([=]{ AppLog::instance().out() << "[CLOUDFILES] Getting " << url << std::endl; });
	}

	CURLcode res = curl_easy_perform(_curl); 
	if (res != CURLE_OK) {
		_ioService.post(std::bind(callback, nullptr));
		throw std::runtime_error(std::string("curl_easy_perform() failed: ") + curl_easy_strerror(res));
	}

	//check the header for an error code
	if (_reqHandler.getLastHttpCode() == 401) {
		if (! isRetry) {
			//we need to re-auth and get a new token, then call ourself again
			_parent.getAuthorizer()->requestNewAuthToken();
			this->performAssetRequest(assetId, callback, true);
			return;
		}
	}

	//handle other errors
	if (_reqHandler.getLastHttpCode() != 200) {
		_ioService.post(std::bind(callback, nullptr));

		if (_reqHandler.getLastHttpCode() != 404) {
			throw std::runtime_error((boost::format("CF HTTP request returned HTTP/%1%") % _reqHandler.getLastHttpCode()).str());
		}
	}
	else
	{
		//no errors, decode
		_reqHandler.getLastBody().seekg(0, std::ios_base::beg);
		
		boost::shared_ptr<InWorldz::Data::Assets::Stratus::StratusAsset> stAsset(new InWorldz::Data::Assets::Stratus::StratusAsset());
		if (! stAsset->ParseFromIstream(&_reqHandler.getLastBody())) {
			_ioService.post(std::bind(callback, nullptr));
			throw std::runtime_error("Unable to deserialize asset " + assetId);
		}

		CloudFilesAsset::ptr cfAsset(new CloudFilesAsset(assetId, stAsset));
		_ioService.post(std::bind(callback, cfAsset));
	}
}


}}