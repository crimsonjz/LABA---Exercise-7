#pragma once

#include "Card.h"
#include "CoreTypes.h"

enum class EGamePhase : uint8
{
	PreFlop,
	Flop,
	Turn,
	River,
	Showdown
};

enum class EPlayerAction : uint8
{
	Fold,
	Check,
	Call,
	Bet,
	Raise,
	AllIn
};

const char* GamePhaseToString(EGamePhase Phase);
const char* PlayerActionToString(EPlayerAction Action);

struct FPlayerDecision
{
	EPlayerAction Action = EPlayerAction::Fold;

	/** For Bet / Raise: the total size of the player's bet for this round ("raise TO"), not the increment. */
	int32 Amount = 0;

	FPlayerDecision() = default;
	explicit FPlayerDecision(EPlayerAction InAction, int32 InAmount = 0) : Action(InAction), Amount(InAmount) {}
};

/** Everything a player is allowed to know when choosing an action. Also the single source of rule validation. */
struct FActionContext
{
	EGamePhase Phase = EGamePhase::PreFlop;
	TArray<FCard> HoleCards;
	TArray<FCard> CommunityCards;

	int32 PlayerChips = 0;
	int32 PlayerCurrentBet = 0;
	int32 HighestBet = 0;
	int32 ToCall = 0;
	int32 MinRaiseTo = 0;
	int32 MaxBetTo = 0;
	int32 TotalPot = 0;
	int32 BigBlind = 0;
	int32 NumOpponentsInHand = 0;

	/** False when only a short all-in has been made since this player last acted. */
	bool bCanRaise = true;

	bool CanCheck() const { return ToCall <= 0; }

	/** Returns true if Decision is legal; otherwise fills OutError with a player-readable reason. */
	bool Validate(const FPlayerDecision& Decision, FString& OutError) const;
};
