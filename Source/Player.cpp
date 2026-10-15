#include "Player.h"

#include <algorithm>
#include <iostream>
#include <sstream>

#include "ConsoleInput.h"

FPlayer::FPlayer(FString InName, int32 InChips)
	: Name(std::move(InName))
	, Chips(InChips)
{
}

void FPlayer::ResetForNewHand()
{
	Hand.clear();
	bIsFolded = false;
	bIsAllIn = false;
	bHasActed = false;
	bIsRaiseClosed = false;
	bIsEliminated = Chips <= 0;
}

int32 FPlayer::RemoveChips(int32 Amount)
{
	const int32 Removed = std::clamp(Amount, 0, Chips);
	Chips -= Removed;
	return Removed;
}

void FPlayer::AddChips(int32 Amount)
{
	if (Amount > 0)
	{
		Chips += Amount;
	}
}

// ---------------------------------------------------------------- FHumanPlayer

FHumanPlayer::FHumanPlayer(FString InName, int32 InChips)
	: FPlayer(std::move(InName), InChips)
{
}

FString FHumanPlayer::BuildOptionsText(const FActionContext& Context)
{
	std::ostringstream Text;
	Text << "  [F]old";
	if (Context.CanCheck())
	{
		Text << "  [C]heck";
	}
	else
	{
		Text << "  [C]all " << std::min(Context.ToCall, Context.PlayerChips);
	}

	const bool bCanOpenOrRaise = Context.PlayerChips > Context.ToCall && Context.bCanRaise;
	if (bCanOpenOrRaise)
	{
		const int32 MinTotal = std::min(Context.MinRaiseTo, Context.MaxBetTo);
		Text << (Context.HighestBet == 0 ? "  [B]et <amount>" : "  [R]aise <to amount>")
			 << " (" << MinTotal << "-" << Context.MaxBetTo << ")";
	}
	Text << "  [A]ll-in " << Context.PlayerChips << "  [Q]uit";
	return Text.str();
}

bool FHumanPlayer::TryParseCommand(const FString& Line, const FActionContext& Context,
	FPlayerDecision& OutDecision, FString& OutError, bool& bOutQuit)
{
	bOutQuit = false;

	std::istringstream Stream(FConsoleInput::ToLower(Line));
	FString Command;
	FString AmountText;
	FString Extra;
	Stream >> Command >> AmountText >> Extra;

	if (Command.empty())
	{
		OutError = "Type a command, e.g. 'c' (call/check), 'f' (fold), 'r 100' (raise to 100), 'a' (all-in).";
		return false;
	}
	if (!Extra.empty())
	{
		OutError = "Too many words - use for example 'r 100'.";
		return false;
	}

	const bool bTakesAmount = (Command == "b" || Command == "bet" || Command == "r" || Command == "raise");
	if (!bTakesAmount && !AmountText.empty())
	{
		OutError = "'" + Command + "' does not take an amount.";
		return false;
	}

	if (Command == "f" || Command == "fold")
	{
		OutDecision = FPlayerDecision(EPlayerAction::Fold);
	}
	else if (Command == "x" || Command == "check")
	{
		OutDecision = FPlayerDecision(EPlayerAction::Check);
	}
	else if (Command == "call")
	{
		OutDecision = FPlayerDecision(EPlayerAction::Call);
	}
	else if (Command == "c")
	{
		OutDecision = FPlayerDecision(Context.CanCheck() ? EPlayerAction::Check : EPlayerAction::Call);
	}
	else if (Command == "a" || Command == "all" || Command == "allin" || Command == "all-in")
	{
		OutDecision = FPlayerDecision(EPlayerAction::AllIn);
	}
	else if (Command == "q" || Command == "quit")
	{
		bOutQuit = true;
		OutDecision = FPlayerDecision(EPlayerAction::Fold);
	}
	else if (bTakesAmount)
	{
		OutDecision = FPlayerDecision(Context.HighestBet == 0 ? EPlayerAction::Bet : EPlayerAction::Raise, INDEX_NONE);
		if (!AmountText.empty())
		{
			int32 Amount = 0;
			if (!FConsoleInput::TryParseInt32(AmountText, Amount))
			{
				OutError = "'" + AmountText + "' is not a valid whole number.";
				return false;
			}
			if (Amount <= 0)
			{
				OutError = "The amount must be a positive whole number.";
				return false;
			}
			OutDecision.Amount = Amount;
		}
	}
	else
	{
		OutError = "Unknown command '" + Command + "'.";
		return false;
	}

	return true;
}

FPlayerDecision FHumanPlayer::DecideAction(const FActionContext& Context)
{
	std::cout << BuildOptionsText(Context) << "\n";

	while (true)
	{
		FString Line;
		if (!FConsoleInput::ReadLine("  " + Name + " > ", Line))
		{
			bQuitRequested = true;
			return FPlayerDecision(EPlayerAction::Fold);
		}

		FPlayerDecision Decision;
		FString Error;
		bool bQuit = false;
		if (!TryParseCommand(Line, Context, Decision, Error, bQuit))
		{
			std::cout << "  " << Error << "\n";
			continue;
		}
		if (bQuit)
		{
			bQuitRequested = true;
			return Decision;
		}

		if (Decision.Amount == INDEX_NONE)
		{
			const int32 MinTotal = std::min(Context.MinRaiseTo, Context.MaxBetTo);
			const FString Label = (Decision.Action == EPlayerAction::Bet) ? "  Bet amount" : "  Raise to";
			if (!FConsoleInput::ReadInt32InRange(Label + " (" + std::to_string(MinTotal) + "-" + std::to_string(Context.MaxBetTo) + "): ",
					1, Context.MaxBetTo, INDEX_NONE, Decision.Amount))
			{
				bQuitRequested = true;
				return FPlayerDecision(EPlayerAction::Fold);
			}
		}

		if (!Context.Validate(Decision, Error))
		{
			std::cout << "  " << Error << "\n";
			continue;
		}
		return Decision;
	}
}

// ---------------------------------------------------------------- FAIPlayer

FAIPlayer::FAIPlayer(FString InName, int32 InChips, std::unique_ptr<IBotStrategy> InStrategy)
	: FPlayer(std::move(InName), InChips)
	, Strategy(std::move(InStrategy))
{
}

FPlayerDecision FAIPlayer::DecideAction(const FActionContext& Context)
{
	if (!Strategy)
	{
		return FPlayerDecision(Context.CanCheck() ? EPlayerAction::Check : EPlayerAction::Fold);
	}
	return Strategy->Decide(Context);
}
