#pragma once

#include <random>

#include "CoreTypes.h"
#include "GameTypes.h"

/** Strategy interface: turns a game snapshot into a decision. */
class IBotStrategy
{
public:
	virtual ~IBotStrategy() = default;
	virtual FPlayerDecision Decide(const FActionContext& Context) = 0;
};

/** Hand-strength + pot-odds bot with a little randomness (bluffs, bet sizing). */
class FBasicBotStrategy : public IBotStrategy
{
public:
	/** Aggression in [0, 1]. Seed 0 means "seed from std::random_device". */
	explicit FBasicBotStrategy(float InAggression = 0.5f, uint32 Seed = 0);

	virtual FPlayerDecision Decide(const FActionContext& Context) override;

	/** Rough hand strength in [0, 1] from hole cards (pre-flop) or best hand + draws (post-flop). */
	static float EstimateHandStrength(const FActionContext& Context);

private:
	float RandomFloat();
	FPlayerDecision MakeRaiseTo(const FActionContext& Context, int32 DesiredTotal) const;

	float Aggression;
	std::mt19937 RandomEngine;
};
