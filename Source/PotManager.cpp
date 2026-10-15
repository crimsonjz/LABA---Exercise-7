#include "PotManager.h"

#include <algorithm>
#include <numeric>

void FPotManager::Reset(int32 NumPlayers)
{
	const size_t Count = static_cast<size_t>(std::max(NumPlayers, 0));
	CurrentBets.assign(Count, 0);
	TotalContributions.assign(Count, 0);
	FoldedFlags.assign(Count, false);
}

bool FPotManager::IsValidIndex(int32 PlayerIndex) const
{
	return PlayerIndex >= 0 && static_cast<size_t>(PlayerIndex) < CurrentBets.size();
}

void FPotManager::PostBet(int32 PlayerIndex, int32 Amount)
{
	if (!IsValidIndex(PlayerIndex) || Amount <= 0)
	{
		return;
	}
	CurrentBets[PlayerIndex] += Amount;
	TotalContributions[PlayerIndex] += Amount;
}

void FPotManager::MarkFolded(int32 PlayerIndex)
{
	if (IsValidIndex(PlayerIndex))
	{
		FoldedFlags[PlayerIndex] = true;
	}
}

void FPotManager::StartNewRound()
{
	std::fill(CurrentBets.begin(), CurrentBets.end(), 0);
}

int32 FPotManager::GetCurrentBet(int32 PlayerIndex) const
{
	return IsValidIndex(PlayerIndex) ? CurrentBets[PlayerIndex] : 0;
}

int32 FPotManager::GetHighestBet() const
{
	return CurrentBets.empty() ? 0 : *std::max_element(CurrentBets.begin(), CurrentBets.end());
}

int32 FPotManager::GetTotalContribution(int32 PlayerIndex) const
{
	return IsValidIndex(PlayerIndex) ? TotalContributions[PlayerIndex] : 0;
}

int32 FPotManager::GetTotalPot() const
{
	return std::accumulate(TotalContributions.begin(), TotalContributions.end(), 0);
}

TArray<FSidePot> FPotManager::BuildPots() const
{
	TArray<int32> Levels;
	for (const int32 Contribution : TotalContributions)
	{
		if (Contribution > 0)
		{
			Levels.push_back(Contribution);
		}
	}
	std::sort(Levels.begin(), Levels.end());
	Levels.erase(std::unique(Levels.begin(), Levels.end()), Levels.end());

	TArray<FSidePot> Pots;
	int32 PreviousLevel = 0;
	int32 CarryOver = 0; // Dead money at a level nobody alive can win

	for (const int32 Level : Levels)
	{
		FSidePot Pot;
		for (size_t Index = 0; Index < TotalContributions.size(); ++Index)
		{
			const int32 Contribution = TotalContributions[Index];
			Pot.Amount += std::min(Contribution, Level) - std::min(Contribution, PreviousLevel);
			if (!FoldedFlags[Index] && Contribution >= Level)
			{
				Pot.EligiblePlayers.push_back(static_cast<int32>(Index));
			}
		}
		PreviousLevel = Level;

		if (Pot.EligiblePlayers.empty())
		{
			CarryOver += Pot.Amount;
			continue;
		}

		Pot.Amount += CarryOver;
		CarryOver = 0;

		if (!Pots.empty() && Pots.back().EligiblePlayers == Pot.EligiblePlayers)
		{
			Pots.back().Amount += Pot.Amount;
		}
		else
		{
			Pots.push_back(std::move(Pot));
		}
	}

	if (CarryOver > 0 && !Pots.empty())
	{
		Pots.back().Amount += CarryOver;
	}

	return Pots;
}

TArray<FPotAward> FPotManager::AwardPots(const TArray<FHandEvaluationResult>& Results, int32 DealerIndex) const
{
	static const FHandEvaluationResult EmptyResult;
	const int32 NumPlayers = static_cast<int32>(TotalContributions.size());

	TArray<FPotAward> Awards;
	for (const FSidePot& Pot : BuildPots())
	{
		TArray<int32> Winners;
		const FHandEvaluationResult* BestResult = nullptr;

		for (const int32 PlayerIndex : Pot.EligiblePlayers)
		{
			const FHandEvaluationResult& Candidate =
				(static_cast<size_t>(PlayerIndex) < Results.size()) ? Results[PlayerIndex] : EmptyResult;

			const int32 Comparison = BestResult ? Candidate.Compare(*BestResult) : 1;
			if (Comparison > 0)
			{
				Winners.assign(1, PlayerIndex);
				BestResult = &Candidate;
			}
			else if (Comparison == 0)
			{
				Winners.push_back(PlayerIndex);
			}
		}

		if (Winners.empty())
		{
			continue;
		}

		// Odd chips go to the first winner clockwise from the dealer button.
		const auto SeatsAfterDealer = [NumPlayers, DealerIndex](int32 Seat)
		{
			return ((Seat - DealerIndex - 1) % NumPlayers + NumPlayers) % NumPlayers;
		};
		std::sort(Winners.begin(), Winners.end(),
			[&SeatsAfterDealer](int32 Left, int32 Right) { return SeatsAfterDealer(Left) < SeatsAfterDealer(Right); });

		const int32 NumWinners = static_cast<int32>(Winners.size());
		const int32 Share = Pot.Amount / NumWinners;
		const int32 Remainder = Pot.Amount % NumWinners;

		FPotAward Award;
		Award.PotAmount = Pot.Amount;
		Award.Winners = Winners;
		for (int32 Index = 0; Index < NumWinners; ++Index)
		{
			Award.Payouts.push_back(Share + (Index < Remainder ? 1 : 0));
		}
		Awards.push_back(std::move(Award));
	}

	return Awards;
}
