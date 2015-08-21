#pragma once

#include <boost/chrono/system_clocks.hpp>

/**
 * Token bucket to rate limit outbound transfers 
 */
class TokenBucket
{
public:
	const static float DRIP_PERIOD;
	const static int DRIP_PERIOD_PERIOD_MS;
	/**
	 * Constructor passing the maximum bandwidth in kB/s and the drip period
	 */
	TokenBucket(int maxBandwidth);
	virtual ~TokenBucket();

	/**
	 * Removes the given number of tokens from the bucket
	 *
	 * \returns True if the tokens could be removed, false if not
	 */
	bool removeTokens(int amount);

	/**
	 * Adds tokens to the bucket based on the amount of time that has passed
	 *
	 * \returns Whether or not tokens have been added
	 */
	bool drip();

	/**
	 * Returns the maximum number of tokens this bucket can hold
	 */
	int getMaxBurst() const;

private:
	int _maxBurst;
	int _tokens;
	boost::chrono::steady_clock::time_point _lastDrip;
	int _tokensPerMS;
};

