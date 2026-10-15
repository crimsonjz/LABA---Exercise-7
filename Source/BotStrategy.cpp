#include "BotStrategy.h"

#include <algorithm>
#include <array>

#include "HandEvaluator.h"

namespace
{
	float EstimatePreFlopStrength(const TArray<FCard>& HoleCards)
	{
		if (HoleCards.size() != 2)
		{
			return 0.2f;
		}

		const int32 HighRank = std::max(HoleCards[0].GetRankValue(), HoleCards[1].GetRankValue());
		const int32 LowRank = std::min(HoleCards[0].GetRankValue(), HoleCards[1].GetRankValue());

		if (HighRank == LowRank)
		{
			return 0.45f + static_cast<float>(HighRank - 2) / 12.0f * 0.55f;
		}

		float Strength = (static_cast<float>(HighRank) * 0.6f + static_cast<float>(LowRank) * 0.4f - 2.0f) / 12.0f * 0.55f;
		if (HoleCards[0].Suit == HoleCards[1].Suit)
		{
			Strength += 0.06f;
		}
		const int32 Gap = HighRank - LowRank;
		if (Gap == 1)
		{
			Strength += 0.05f;
		}
		else if (Gap == 2)
		{
			Strength += 0.02f;
		}
		return std::clamp(Strength, 0.0f, 1.0f);
	}

	float EstimateDrawBonus(const TArray<FCard>& HoleCards, const TArray<FCard>& AllCards)
	{
		float Bonus = 0.0f;

		std::array<int32, 4> SuitCounts{};
		std::array<int32, 4> HoleSuitCounts{};
		for (const FCard& Card : AllCards)
		{
			++SuitCounts[static_cast<size_t>(Card.Suit)];
		}
		for (const FCard& Card : HoleCards)
		{
			++HoleSuitCounts[static_cast<size_t>(Card.Suit)];
		}
		for (size_t Suit = 0; Suit < 4; ++Suit)
		{
			if (SuitCounts[Suit] == 4 && HoleSuitCounts[Suit] > 0)
			{
				Bonus += 0.15f;
			}
		}

		std::array<bool, 15> HasRank{};
		for (const FCard& Card : AllCards)
		{
			HasRank[Card.GetRankValue()] = true;
		}
		HasRank[1] = HasRank[14];
		for (int32 Low = 1; Low <= 10; ++Low)
		{
			int32 Present = 0;
			for (int32 Offset = 0; Offset < 5; ++Offset)
			{
				Present += HasRank[Low + Offset] ? 1 : 0;
			}
			if (Present == 4)
			{
				Bonus += 0.08f;
				break;
			}
		}

		return Bonus;
	}
}

FBasicBotStrategy::FBasicBotStrategy(float InAggression, uint32 Seed)
	: Aggression(std::clamp(InAggression, 0.0f, 1.0f))
	, RandomEngine(Seed != 0 ? Seed : std::random_device{}())
{
}

float FBasicBotStrategy::RandomFloat()
{
	return std::uniform_real_distribution<float>(0.0f, 1.0f)(RandomEngine);
}

float FBasicBotStrategy::EstimateHandStrength(const FActionContext& Context)
{
	if (Context.CommunityCards.empty())
	{
		return EstimatePreFlopStrength(Context.HoleCards);
	}

	TArray<FCard> AllCards = Context.HoleCards;
	AllCards.insert(AllCards.end(), Context.CommunityCards.begin(), Context.CommunityCards.end());
	if (AllCards.size() < 5)
	{
		return EstimatePreFlopStrength(Context.HoleCards);
	}

	const FHandEvaluationResult Evaluation = FHandEvaluator::Evaluate(AllCards);
	float Strength = 0.0f;

	switch (Evaluation.HandRank)
	{
	case EHandRank::HighCard:
		Strength = 0.05f + static_cast<float>(Evaluation.Kickers[0] - 2) / 12.0f * 0.15f;
		break;
	case EHandRank::OnePair:
	{
		const uint8 PairRank = Evaluation.Kickers[0];
		const bool bUsesHoleCard = std::any_of(Context.HoleCards.begin(), Context.HoleCards.end(),
			[PairRank](const FCard& Card) { return Card.GetRankValue() == PairRank; });
		Strength = bUsesHoleCard ? 0.40f + static_cast<float>(PairRank - 2) / 12.0f * 0.20f : 0.22f;
		break;
	}
	case EHandRank::TwoPair:       Strength = 0.72f; break;
	case EHandRank::ThreeOfAKind:  Strength = 0.80f; break;
	case EHandRank::Straight:      Strength = 0.87f; break;
	case EHandRank::Flush:         Strength = 0.90f; break;
	case EHandRank::FullHouse:     Strength = 0.95f; break;
	default:                       Strength = 0.99f; break;
	}

	if (Context.Phase == EGamePhase::Flop || Context.Phase == EGamePhase::Turn)
	{
		if (Evaluation.HandRank <= EHandRank::OnePair)
		{
			Strength += EstimateDrawBonus(Context.HoleCards, AllCards);
		}
	}

	return std::clamp(Strength, 0.0f, 1.0f);
}

FPlayerDecision FBasicBotStrategy::MakeRaiseTo(const FActionContext& Context, int32 DesiredTotal) const
{
	if (!Context.bCanRaise || Context.MaxBetTo <= Context.HighestBet)
	{
		return FPlayerDecision(Context.CanCheck() ? EPlayerAction::Check : EPlayerAction::Call);
	}

	// Short stacks (or huge bets relative to the stack) simply shove.
	if (DesiredTotal >= Context.MaxBetTo || Context.MaxBetTo < Context.MinRaiseTo)
	{
		return FPlayerDecision(EPlayerAction::AllIn);
	}

	const int32 Total = std::clamp(DesiredTotal, Context.MinRaiseTo, Context.MaxBetTo);
	if (Total >= Context.MaxBetTo)
	{
		return FPlayerDecision(EPlayerAction::AllIn);
	}
	return FPlayerDecision(Context.HighestBet == 0 ? EPlayerAction::Bet : EPlayerAction::Raise, Total);
}

FPlayerDecision FBasicBotStrategy::Decide(const FActionContext& Context)
{
	const float Noise = (RandomFloat() - 0.5f) * 0.10f;
	const float Strength = std::clamp(EstimateHandStrength(Context) + Noise, 0.0f, 1.0f);
	const bool bBluffing = RandomFloat() < 0.04f + 0.08f * Aggression;
	const int32 Pot = std::max(Context.TotalPot, Context.BigBlind);

	if (Context.CanCheck())
	{
		if (Strength > 0.70f || bBluffing)
		{
			const float PotFraction = 0.5f + 0.3f * RandomFloat() + 0.2f * Aggression;
			const int32 Target = Context.PlayerCurrentBet + std::max(Context.BigBlind, static_cast<int32>(static_cast<float>(Pot) * PotFraction));
			return MakeRaiseTo(Context, Context.HighestBet == 0 ? Target - Context.PlayerCurrentBet : Target);
		}
		if (Strength > 0.50f && RandomFloat() < 0.35f + 0.3f * Aggression)
		{
			const int32 Target = std::max(Context.BigBlind, static_cast<int32>(static_cast<float>(Pot) * 0.4f));
			return MakeRaiseTo(Context, Context.HighestBet == 0 ? Target : Context.HighestBet + Target);
		}
		return FPlayerDecision(EPlayerAction::Check);
	}

	const float PotOdds = static_cast<float>(Context.ToCall) / static_cast<float>(Context.TotalPot + Context.ToCall);

	if (Strength > 0.82f && RandomFloat() < 0.55f + 0.35f * Aggression)
	{
		const int32 RaiseSize = std::max(Context.MinRaiseTo - Context.HighestBet, static_cast<int32>(static_cast<float>(Pot) * 0.75f));
		const FPlayerDecision Raise = MakeRaiseTo(Context, Context.HighestBet + RaiseSize);
		return Raise;
	}

	if (Strength >= PotOdds + 0.10f)
	{
		return FPlayerDecision(EPlayerAction::Call);
	}

	// Cheap calls with marginal hands / occasional bluff-calls.
	const bool bCheapCall = Context.ToCall <= Context.BigBlind && Strength >= PotOdds;
	if (bCheapCall || (bBluffing && Context.ToCall * 4 <= Context.PlayerChips))
	{
		return FPlayerDecision(EPlayerAction::Call);
	}

	return FPlayerDecision(EPlayerAction::Fold);
}
