#include "stdafx.h"
#include "TokenBucket.h"

namespace bc = boost::chrono;

const float TokenBucket::DRIP_PERIOD = 0.0156f;
const int TokenBucket::DRIP_PERIOD_PERIOD_MS = 15;

TokenBucket::TokenBucket(int maxBandwidth)
	: _maxBurst(maxBandwidth), _tokens(_maxBurst), _lastDrip(bc::steady_clock::now()),
	_tokensPerMS(maxBandwidth / 1000)
{
	if (_tokensPerMS <= 0)
	{
		_tokensPerMS = 1;
	}
}

TokenBucket::~TokenBucket()
{
}

bool TokenBucket::removeTokens(int amount)
{
	this->drip();

	if (_tokens >= amount) {
		_tokens -= amount;
		return true;
	}

	return false;
}

bool TokenBucket::drip()
{
	int deltaTime = bc::duration_cast<bc::milliseconds>(bc::steady_clock::now() - _lastDrip).count();
	if (deltaTime <= 0)
	{
		return false;
	}

	int tokens = _tokensPerMS * deltaTime;
	_tokens = std::min(_tokens + tokens, _maxBurst);

	_lastDrip = bc::steady_clock::now();
	return true;
}

int TokenBucket::getMaxBurst() const
{
	return _maxBurst;
}
