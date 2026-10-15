#pragma once

#include "Card.h"
#include "CoreTypes.h"

enum class EHandRank : uint8
{
	HighCard,
	OnePair,
	TwoPair,
	ThreeOfAKind,
	Straight,
	Flush,
	FullHouse,
	FourOfAKind,
	StraightFlush,
	RoyalFlush
};

const char* GetHandRankName(EHandRank HandRank);

struct FHandEvaluationResult
{
	EHandRank HandRank = EHandRank::HighCard;

	/**
	 * Tie-break values ordered by significance, always the same length for a given HandRank.
	 * e.g. TwoPair: {HighPair, LowPair, Kicker}; FullHouse: {Trips, Pair}; Straight: {HighCard (5 for the wheel)}.
	 */
	TArray<uint8> Kickers;

	/** The five cards that make up the hand, in significance order. */
	TArray<FCard> BestFive;

	/** > 0 if this hand beats Other, < 0 if it loses, 0 for an exact tie. */
	int32 Compare(const FHandEvaluationResult& Other) const;

	bool operator==(const FHandEvaluationResult& Other) const { return Compare(Other) == 0; }
	bool operator<(const FHandEvaluationResult& Other) const { return Compare(Other) < 0; }

	/** e.g. "Two Pair, Kings and Sevens". */
	FString ToString() const;
};

class FHandEvaluator
{
public:
	/** Best 5-card hand out of 5..N (typically 7) cards. Fewer than 5 cards yields an empty High Card result. */
	static FHandEvaluationResult Evaluate(const TArray<FCard>& Cards);

	/** Evaluates exactly five cards. */
	static FHandEvaluationResult EvaluateFive(const TArray<FCard>& Five);
};
