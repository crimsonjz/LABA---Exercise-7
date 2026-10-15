#include "HandEvaluator.h"

#include <algorithm>
#include <array>
#include <bit>

namespace
{
	const char* GetRankName(uint8 RankValue)
	{
		static const char* const Names[] = {
			"Two", "Three", "Four", "Five", "Six", "Seven", "Eight",
			"Nine", "Ten", "Jack", "Queen", "King", "Ace"};
		return (RankValue >= 2 && RankValue <= 14) ? Names[RankValue - 2] : "?";
	}

	FString GetRankPlural(uint8 RankValue)
	{
		FString Name = GetRankName(RankValue);
		Name += (RankValue == 6) ? "es" : "s";
		return Name;
	}

	struct FRankGroup
	{
		uint8 Rank;
		int32 Count;
	};
}

const char* GetHandRankName(EHandRank HandRank)
{
	switch (HandRank)
	{
	case EHandRank::HighCard:      return "High Card";
	case EHandRank::OnePair:       return "One Pair";
	case EHandRank::TwoPair:       return "Two Pair";
	case EHandRank::ThreeOfAKind:  return "Three of a Kind";
	case EHandRank::Straight:      return "Straight";
	case EHandRank::Flush:         return "Flush";
	case EHandRank::FullHouse:     return "Full House";
	case EHandRank::FourOfAKind:   return "Four of a Kind";
	case EHandRank::StraightFlush: return "Straight Flush";
	case EHandRank::RoyalFlush:    return "Royal Flush";
	}
	return "?";
}

int32 FHandEvaluationResult::Compare(const FHandEvaluationResult& Other) const
{
	if (HandRank != Other.HandRank)
	{
		return HandRank > Other.HandRank ? 1 : -1;
	}

	const size_t Count = std::min(Kickers.size(), Other.Kickers.size());
	for (size_t Index = 0; Index < Count; ++Index)
	{
		if (Kickers[Index] != Other.Kickers[Index])
		{
			return Kickers[Index] > Other.Kickers[Index] ? 1 : -1;
		}
	}
	return 0;
}

FString FHandEvaluationResult::ToString() const
{
	if (Kickers.empty())
	{
		return GetHandRankName(HandRank);
	}

	const FString Prefix = FString(GetHandRankName(HandRank));
	switch (HandRank)
	{
	case EHandRank::HighCard:      return Prefix + ", " + GetRankName(Kickers[0]) + " high";
	case EHandRank::OnePair:       return "Pair of " + GetRankPlural(Kickers[0]);
	case EHandRank::TwoPair:       return Prefix + ", " + GetRankPlural(Kickers[0]) + " and " + GetRankPlural(Kickers[1]);
	case EHandRank::ThreeOfAKind:  return Prefix + ", " + GetRankPlural(Kickers[0]);
	case EHandRank::Straight:      return Prefix + ", " + GetRankName(Kickers[0]) + " high";
	case EHandRank::Flush:         return Prefix + ", " + GetRankName(Kickers[0]) + " high";
	case EHandRank::FullHouse:     return Prefix + ", " + GetRankPlural(Kickers[0]) + " full of " + GetRankPlural(Kickers[1]);
	case EHandRank::FourOfAKind:   return Prefix + ", " + GetRankPlural(Kickers[0]);
	case EHandRank::StraightFlush: return Prefix + ", " + GetRankName(Kickers[0]) + " high";
	case EHandRank::RoyalFlush:    return Prefix;
	}
	return Prefix;
}

FHandEvaluationResult FHandEvaluator::EvaluateFive(const TArray<FCard>& Five)
{
	FHandEvaluationResult Result;
	if (Five.size() != 5)
	{
		return Result;
	}

	std::array<int32, 15> RankCounts{};
	bool bIsFlush = true;
	for (const FCard& Card : Five)
	{
		++RankCounts[Card.GetRankValue()];
		if (Card.Suit != Five[0].Suit)
		{
			bIsFlush = false;
		}
	}

	// Groups ordered by (count desc, rank desc): the natural tie-break order for paired hands.
	TArray<FRankGroup> Groups;
	for (int32 RankValue = 14; RankValue >= 2; --RankValue)
	{
		if (RankCounts[RankValue] > 0)
		{
			Groups.push_back({static_cast<uint8>(RankValue), RankCounts[RankValue]});
		}
	}
	std::stable_sort(Groups.begin(), Groups.end(),
		[](const FRankGroup& Left, const FRankGroup& Right) { return Left.Count > Right.Count; });

	uint8 StraightHigh = 0;
	if (Groups.size() == 5)
	{
		if (Groups[0].Rank - Groups[4].Rank == 4)
		{
			StraightHigh = Groups[0].Rank;
		}
		else if (Groups[0].Rank == 14 && Groups[1].Rank == 5 && Groups[4].Rank == 2)
		{
			StraightHigh = 5; // Wheel: A-2-3-4-5
		}
	}

	for (const FRankGroup& Group : Groups)
	{
		Result.Kickers.push_back(Group.Rank);
	}

	if (StraightHigh != 0 && bIsFlush)
	{
		Result.HandRank = (StraightHigh == 14) ? EHandRank::RoyalFlush : EHandRank::StraightFlush;
		Result.Kickers = {StraightHigh};
	}
	else if (Groups[0].Count == 4)
	{
		Result.HandRank = EHandRank::FourOfAKind;
	}
	else if (Groups[0].Count == 3 && Groups[1].Count == 2)
	{
		Result.HandRank = EHandRank::FullHouse;
	}
	else if (bIsFlush)
	{
		Result.HandRank = EHandRank::Flush;
	}
	else if (StraightHigh != 0)
	{
		Result.HandRank = EHandRank::Straight;
		Result.Kickers = {StraightHigh};
	}
	else if (Groups[0].Count == 3)
	{
		Result.HandRank = EHandRank::ThreeOfAKind;
	}
	else if (Groups[0].Count == 2 && Groups[1].Count == 2)
	{
		Result.HandRank = EHandRank::TwoPair;
	}
	else if (Groups[0].Count == 2)
	{
		Result.HandRank = EHandRank::OnePair;
	}
	else
	{
		Result.HandRank = EHandRank::HighCard;
	}

	// Order the cards by significance (grouped ranks first); the wheel's Ace plays low, so it goes last.
	Result.BestFive = Five;
	std::stable_sort(Result.BestFive.begin(), Result.BestFive.end(),
		[&RankCounts, StraightHigh](const FCard& Left, const FCard& Right)
		{
			if (RankCounts[Left.GetRankValue()] != RankCounts[Right.GetRankValue()])
			{
				return RankCounts[Left.GetRankValue()] > RankCounts[Right.GetRankValue()];
			}
			const uint8 LeftValue = (StraightHigh == 5 && Left.GetRankValue() == 14) ? 1 : Left.GetRankValue();
			const uint8 RightValue = (StraightHigh == 5 && Right.GetRankValue() == 14) ? 1 : Right.GetRankValue();
			return LeftValue > RightValue;
		});

	return Result;
}

FHandEvaluationResult FHandEvaluator::Evaluate(const TArray<FCard>& Cards)
{
	const uint32 NumCards = static_cast<uint32>(Cards.size());
	if (NumCards < 5 || NumCards > 16)
	{
		return FHandEvaluationResult();
	}

	FHandEvaluationResult Best;
	bool bHasBest = false;
	TArray<FCard> Five;
	Five.reserve(5);

	for (uint32 Mask = 0; Mask < (1u << NumCards); ++Mask)
	{
		if (std::popcount(Mask) != 5)
		{
			continue;
		}

		Five.clear();
		for (uint32 Index = 0; Index < NumCards; ++Index)
		{
			if (Mask & (1u << Index))
			{
				Five.push_back(Cards[Index]);
			}
		}

		FHandEvaluationResult Candidate = EvaluateFive(Five);
		if (!bHasBest || Candidate.Compare(Best) > 0)
		{
			Best = std::move(Candidate);
			bHasBest = true;
		}
	}

	return Best;
}
