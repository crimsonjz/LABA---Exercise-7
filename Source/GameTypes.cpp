#include "GameTypes.h"

const char* GamePhaseToString(EGamePhase Phase)
{
	switch (Phase)
	{
	case EGamePhase::PreFlop:  return "Pre-Flop";
	case EGamePhase::Flop:     return "Flop";
	case EGamePhase::Turn:     return "Turn";
	case EGamePhase::River:    return "River";
	case EGamePhase::Showdown: return "Showdown";
	}
	return "?";
}

const char* PlayerActionToString(EPlayerAction Action)
{
	switch (Action)
	{
	case EPlayerAction::Fold:  return "Fold";
	case EPlayerAction::Check: return "Check";
	case EPlayerAction::Call:  return "Call";
	case EPlayerAction::Bet:   return "Bet";
	case EPlayerAction::Raise: return "Raise";
	case EPlayerAction::AllIn: return "All-in";
	}
	return "?";
}

bool FActionContext::Validate(const FPlayerDecision& Decision, FString& OutError) const
{
	switch (Decision.Action)
	{
	case EPlayerAction::Fold:
		return true;

	case EPlayerAction::Check:
		if (ToCall > 0)
		{
			OutError = "You cannot check - there is a bet of " + std::to_string(ToCall) + " to call.";
			return false;
		}
		return true;

	case EPlayerAction::Call:
		if (ToCall <= 0)
		{
			OutError = "There is nothing to call - you can check.";
			return false;
		}
		return true;

	case EPlayerAction::Bet:
		if (HighestBet > 0)
		{
			OutError = "There is already a bet this round - you can only call or raise.";
			return false;
		}
		if (Decision.Amount <= 0)
		{
			OutError = "A bet must be a positive amount.";
			return false;
		}
		if (Decision.Amount > PlayerChips)
		{
			OutError = "You only have " + std::to_string(PlayerChips) + " chips.";
			return false;
		}
		if (Decision.Amount < BigBlind && Decision.Amount != PlayerChips)
		{
			OutError = "The minimum bet is " + std::to_string(BigBlind) + " (the big blind).";
			return false;
		}
		return true;

	case EPlayerAction::Raise:
		if (HighestBet <= 0)
		{
			OutError = "There is no bet to raise - open with a bet instead.";
			return false;
		}
		if (!bCanRaise)
		{
			OutError = "Betting was not re-opened for you - you can only call or fold.";
			return false;
		}
		if (Decision.Amount > MaxBetTo)
		{
			OutError = "You can raise to at most " + std::to_string(MaxBetTo) + " (your whole stack).";
			return false;
		}
		if (Decision.Amount <= HighestBet)
		{
			OutError = "A raise must exceed the current bet of " + std::to_string(HighestBet) + ".";
			return false;
		}
		if (Decision.Amount < MinRaiseTo && Decision.Amount != MaxBetTo)
		{
			OutError = "The minimum raise is to " + std::to_string(MinRaiseTo) + ".";
			return false;
		}
		return true;

	case EPlayerAction::AllIn:
		if (PlayerChips <= 0)
		{
			OutError = "You have no chips left.";
			return false;
		}
		if (MaxBetTo > HighestBet && !bCanRaise && HighestBet > 0)
		{
			OutError = "Betting was not re-opened for you - you can only call or fold.";
			return false;
		}
		return true;
	}

	OutError = "Unknown action.";
	return false;
}
