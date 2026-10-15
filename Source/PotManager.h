#pragma once

#include "CoreTypes.h"
#include "HandEvaluator.h"

/** One (main or side) pot and the seats allowed to win it. */
struct FSidePot
{
	int32 Amount = 0;
	TArray<int32> EligiblePlayers;
};

/** Result of awarding a single pot. Winners and Payouts are parallel arrays. */
struct FPotAward
{
	int32 PotAmount = 0;
	TArray<int32> Winners;
	TArray<int32> Payouts;
};

/**
 * Tracks chips committed this betting round and over the whole hand, builds main/side pots
 * from the contribution levels and splits them between winners.
 */
class FPotManager
{
public:
	void Reset(int32 NumPlayers);

	/** Records Amount chips going into the pot from PlayerIndex. */
	void PostBet(int32 PlayerIndex, int32 Amount);

	void MarkFolded(int32 PlayerIndex);

	/** Clears the per-round bets (total contributions stay). */
	void StartNewRound();

	int32 GetCurrentBet(int32 PlayerIndex) const;
	int32 GetHighestBet() const;
	int32 GetTotalContribution(int32 PlayerIndex) const;
	int32 GetTotalPot() const;

	/** Main pot first, then side pots. Uncalled chips come back as a pot only their owner is eligible for. */
	TArray<FSidePot> BuildPots() const;

	/**
	 * Splits every pot between the best eligible hands. Results is indexed by seat; odd chips go to the
	 * winner closest to the left of DealerIndex.
	 */
	TArray<FPotAward> AwardPots(const TArray<FHandEvaluationResult>& Results, int32 DealerIndex) const;

private:
	bool IsValidIndex(int32 PlayerIndex) const;

	TArray<int32> CurrentBets;
	TArray<int32> TotalContributions;
	TArray<bool> FoldedFlags;
};
