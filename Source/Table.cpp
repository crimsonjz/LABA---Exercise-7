#include "Table.h"

#include <algorithm>
#include <iostream>

#include "ConsoleInput.h"
#include "HandEvaluator.h"

namespace
{
	constexpr size_t MaxRecentActions = 6;
}

FTable::FTable(const FTableConfig& InConfig)
	: Config(InConfig)
{
}

void FTable::AddPlayer(std::unique_ptr<FPlayer> Player)
{
	if (Player)
	{
		Players.push_back(std::move(Player));
	}
}

// ---------------------------------------------------------------- Game loop

void FTable::Run()
{
	Renderer.PrintBanner("TEXAS HOLD'EM - NO LIMIT");
	for (const std::unique_ptr<FPlayer>& Player : Players)
	{
		Player->ResetForNewHand();
	}

	while (!bQuitRequested && CountActivePlayers() >= 2)
	{
		PlayHand();
		if (bQuitRequested)
		{
			break;
		}

		ReportEliminations();

		const bool bHumanBusted = std::any_of(Players.begin(), Players.end(),
			[](const std::unique_ptr<FPlayer>& Player) { return Player->IsHuman() && Player->Chips <= 0; });
		if (bHumanBusted)
		{
			Renderer.PrintBanner("You are out of chips - game over.");
			return;
		}
		if (CountActivePlayers() < 2)
		{
			break;
		}

		FString Line;
		if (!FConsoleInput::ReadLine("\n  Press Enter for the next hand (q to quit): ", Line)
			|| FConsoleInput::ToLower(Line) == "q" || FConsoleInput::ToLower(Line) == "quit")
		{
			bQuitRequested = true;
		}
	}

	if (CountActivePlayers() == 1)
	{
		for (const std::unique_ptr<FPlayer>& Player : Players)
		{
			if (Player->Chips > 0)
			{
				Renderer.PrintBanner(Player->Name + " wins the game with " + std::to_string(Player->Chips) + " chips!");
			}
		}
	}
	else
	{
		Renderer.PrintBanner("Game ended. Thanks for playing!");
		for (const std::unique_ptr<FPlayer>& Player : Players)
		{
			Renderer.PrintLine("  " + Player->Name + ": " + std::to_string(Player->Chips));
		}
	}
}

void FTable::PlayHand()
{
	StartHand();
	ShowTable();

	const int32 NumActive = CountActivePlayers();
	const int32 PreFlopStart = (NumActive == 2) ? DealerIndex : NextSeat(BigBlindIndex, false);
	RunBettingRound(PreFlopStart, true);

	const EGamePhase Streets[] = {EGamePhase::Flop, EGamePhase::Turn, EGamePhase::River};
	for (const EGamePhase Street : Streets)
	{
		if (bQuitRequested || CountPlayersInHand() <= 1)
		{
			break;
		}
		DealCommunityCards(Street, Street == EGamePhase::Flop ? 3 : 1);
		ShowTable();
		RunBettingRound(NextSeat(DealerIndex, true), false);
	}

	if (!bQuitRequested)
	{
		ResolveHand();
	}
}

void FTable::StartHand()
{
	++HandNumber;
	Deck.Reset();
	Deck.Shuffle();
	CommunityCards.clear();
	RecentActions.clear();
	bRevealAllHands = false;
	CurrentActorIndex = INDEX_NONE;

	for (const std::unique_ptr<FPlayer>& Player : Players)
	{
		Player->ResetForNewHand();
	}

	const int32 NumSeats = static_cast<int32>(Players.size());
	Pot.Reset(NumSeats);

	DealerIndex = NextSeat(DealerIndex == INDEX_NONE ? NumSeats - 1 : DealerIndex, false);
	if (CountActivePlayers() == 2)
	{
		// Heads-up: the dealer posts the small blind.
		SmallBlindIndex = DealerIndex;
		BigBlindIndex = NextSeat(DealerIndex, false);
	}
	else
	{
		SmallBlindIndex = NextSeat(DealerIndex, false);
		BigBlindIndex = NextSeat(SmallBlindIndex, false);
	}

	Phase = EGamePhase::PreFlop;
	MinRaiseSize = Config.BigBlind;

	Renderer.PrintBanner("Hand #" + std::to_string(HandNumber));
	PostBlind(SmallBlindIndex, Config.SmallBlind, "small blind");
	PostBlind(BigBlindIndex, Config.BigBlind, "big blind");

	const int32 NumActive = CountActivePlayers();
	for (int32 Pass = 0; Pass < 2; ++Pass)
	{
		int32 Seat = SmallBlindIndex;
		for (int32 Count = 0; Count < NumActive; ++Count)
		{
			Players[Seat]->Hand.push_back(Deck.Draw());
			Seat = NextSeat(Seat, false);
		}
	}
}

void FTable::PostBlind(int32 Seat, int32 Amount, const char* Label)
{
	FPlayer& Player = *Players[Seat];
	const int32 Posted = std::min(Amount, Player.Chips);
	CommitChips(Seat, Posted);
	LogEvent(Player.Name + " posts the " + Label + " (" + std::to_string(Posted) + ")");
}

void FTable::DealCommunityCards(EGamePhase NewPhase, int32 NumCards)
{
	Phase = NewPhase;
	Deck.Draw(); // Burn card
	for (int32 Index = 0; Index < NumCards; ++Index)
	{
		CommunityCards.push_back(Deck.Draw());
	}
	Renderer.PrintBanner(GamePhaseToString(NewPhase));
}

// ---------------------------------------------------------------- Betting

bool FTable::NeedsToAct(int32 Seat) const
{
	const FPlayer& Player = *Players[Seat];
	if (Player.bIsEliminated || Player.bIsFolded || Player.bIsAllIn)
	{
		return false;
	}
	return !Player.bHasActed || Pot.GetCurrentBet(Seat) < Pot.GetHighestBet();
}

void FTable::RunBettingRound(int32 FirstSeat, bool bPreFlop)
{
	if (!bPreFlop)
	{
		Pot.StartNewRound();
	}
	MinRaiseSize = Config.BigBlind;
	for (const std::unique_ptr<FPlayer>& Player : Players)
	{
		Player->bHasActed = false;
		Player->bIsRaiseClosed = false;
	}

	const int32 NumSeats = static_cast<int32>(Players.size());
	int32 SearchSeat = FirstSeat;

	while (!bQuitRequested && CountPlayersInHand() > 1)
	{
		const int32 NumAbleToAct = CountPlayersAbleToAct();
		if (NumAbleToAct == 0)
		{
			break;
		}

		int32 Seat = INDEX_NONE;
		for (int32 Offset = 0; Offset < NumSeats; ++Offset)
		{
			const int32 Candidate = (SearchSeat + Offset) % NumSeats;
			if (NeedsToAct(Candidate))
			{
				Seat = Candidate;
				break;
			}
		}
		if (Seat == INDEX_NONE)
		{
			break;
		}

		// A lone player who has already matched the bet has nobody left to bet against.
		if (NumAbleToAct == 1 && Pot.GetCurrentBet(Seat) >= Pot.GetHighestBet())
		{
			break;
		}

		FPlayer& Player = *Players[Seat];
		CurrentActorIndex = Seat;
		if (Player.IsHuman() && bTableDirty)
		{
			ShowTable();
		}

		const FActionContext Context = BuildContext(Seat);
		FPlayerDecision Decision = Player.DecideAction(Context);
		if (Player.WantsToQuit())
		{
			bQuitRequested = true;
			break;
		}

		FString Error;
		if (!Context.Validate(Decision, Error))
		{
			Decision = FPlayerDecision(Context.CanCheck() ? EPlayerAction::Check : EPlayerAction::Fold);
		}

		ApplyDecision(Seat, Decision);
		Player.bHasActed = true;
		SearchSeat = (Seat + 1) % NumSeats;
	}

	CurrentActorIndex = INDEX_NONE;
}

FActionContext FTable::BuildContext(int32 Seat) const
{
	const FPlayer& Player = *Players[Seat];

	FActionContext Context;
	Context.Phase = Phase;
	Context.HoleCards = Player.Hand;
	Context.CommunityCards = CommunityCards;
	Context.PlayerChips = Player.Chips;
	Context.PlayerCurrentBet = Pot.GetCurrentBet(Seat);
	Context.HighestBet = Pot.GetHighestBet();
	Context.ToCall = std::max(Context.HighestBet - Context.PlayerCurrentBet, 0);
	Context.MinRaiseTo = Context.HighestBet + MinRaiseSize;
	Context.MaxBetTo = Context.PlayerCurrentBet + Player.Chips;
	Context.TotalPot = Pot.GetTotalPot();
	Context.BigBlind = Config.BigBlind;
	Context.NumOpponentsInHand = CountPlayersInHand() - 1;
	Context.bCanRaise = !Player.bIsRaiseClosed;
	return Context;
}

void FTable::CommitChips(int32 Seat, int32 Amount)
{
	FPlayer& Player = *Players[Seat];
	const int32 Removed = Player.RemoveChips(Amount);
	Pot.PostBet(Seat, Removed);
	if (Player.Chips == 0)
	{
		Player.bIsAllIn = true;
	}
}

void FTable::RaiseTo(int32 Seat, int32 TotalBet)
{
	FPlayer& Player = *Players[Seat];
	const int32 PreviousHighest = Pot.GetHighestBet();
	CommitChips(Seat, TotalBet - Pot.GetCurrentBet(Seat));

	const int32 RaiseSize = TotalBet - PreviousHighest;
	const bool bFullRaise = RaiseSize >= MinRaiseSize;

	for (size_t Index = 0; Index < Players.size(); ++Index)
	{
		FPlayer& Other = *Players[Index];
		if (static_cast<int32>(Index) == Seat || Other.bIsEliminated || Other.bIsFolded || Other.bIsAllIn)
		{
			continue;
		}
		if (bFullRaise)
		{
			Other.bIsRaiseClosed = false;
		}
		else if (Other.bHasActed)
		{
			// A short all-in does not re-open the betting for players who already acted.
			Other.bIsRaiseClosed = true;
		}
		Other.bHasActed = false;
	}
	if (bFullRaise)
	{
		MinRaiseSize = RaiseSize;
	}

	FString Message = Player.Name + (PreviousHighest == 0 ? " bets " : " raises to ") + std::to_string(TotalBet);
	if (Player.bIsAllIn)
	{
		Message += " (ALL-IN)";
	}
	LogEvent(Message);
}

void FTable::ApplyDecision(int32 Seat, const FPlayerDecision& Decision)
{
	FPlayer& Player = *Players[Seat];
	const int32 ToCall = std::max(Pot.GetHighestBet() - Pot.GetCurrentBet(Seat), 0);

	switch (Decision.Action)
	{
	case EPlayerAction::Fold:
		Player.bIsFolded = true;
		Pot.MarkFolded(Seat);
		LogEvent(Player.Name + " folds");
		break;

	case EPlayerAction::Check:
		LogEvent(Player.Name + " checks");
		break;

	case EPlayerAction::Call:
	{
		const int32 Amount = std::min(ToCall, Player.Chips);
		CommitChips(Seat, Amount);
		LogEvent(Player.Name + " calls " + std::to_string(Amount) + (Player.bIsAllIn ? " (ALL-IN)" : ""));
		break;
	}

	case EPlayerAction::Bet:
	case EPlayerAction::Raise:
		RaiseTo(Seat, Decision.Amount);
		break;

	case EPlayerAction::AllIn:
	{
		const int32 Target = Pot.GetCurrentBet(Seat) + Player.Chips;
		if (Target > Pot.GetHighestBet())
		{
			RaiseTo(Seat, Target);
		}
		else
		{
			const int32 Amount = Player.Chips;
			CommitChips(Seat, Amount);
			LogEvent(Player.Name + " calls " + std::to_string(Amount) + " (ALL-IN)");
		}
		break;
	}
	}
}

// ---------------------------------------------------------------- Showdown

void FTable::ResolveHand()
{
	TArray<FHandEvaluationResult> Results(Players.size());

	if (CountPlayersInHand() > 1)
	{
		Phase = EGamePhase::Showdown;
		bRevealAllHands = true;
		CurrentActorIndex = INDEX_NONE;
		ShowTable();

		Renderer.PrintBanner("Showdown");
		for (size_t Index = 0; Index < Players.size(); ++Index)
		{
			const FPlayer& Player = *Players[Index];
			if (Player.bIsEliminated || Player.bIsFolded)
			{
				continue;
			}

			TArray<FCard> AllCards = Player.Hand;
			AllCards.insert(AllCards.end(), CommunityCards.begin(), CommunityCards.end());
			Results[Index] = FHandEvaluator::Evaluate(AllCards);

			FString Cards;
			for (const FCard& Card : Player.Hand)
			{
				Cards += "[" + Card.ToDisplayString() + "]";
			}
			Renderer.PrintLine("  " + Player.Name + ": " + Cards + "  " + Results[Index].ToString());
		}
	}

	AwardPots(Results);
}

void FTable::AwardPots(const TArray<FHandEvaluationResult>& Results)
{
	const TArray<FPotAward> Awards = Pot.AwardPots(Results, DealerIndex);

	Renderer.PrintLine("");
	for (size_t PotIndex = 0; PotIndex < Awards.size(); ++PotIndex)
	{
		const FPotAward& Award = Awards[PotIndex];
		const FString PotName = (PotIndex == 0) ? "Main pot" : "Side pot " + std::to_string(PotIndex);

		for (size_t WinnerIndex = 0; WinnerIndex < Award.Winners.size(); ++WinnerIndex)
		{
			FPlayer& Winner = *Players[Award.Winners[WinnerIndex]];
			Winner.AddChips(Award.Payouts[WinnerIndex]);

			FString Line = "  " + PotName + " (" + std::to_string(Award.PotAmount) + "): " + Winner.Name + " wins "
				+ std::to_string(Award.Payouts[WinnerIndex]);
			if (Award.Winners.size() > 1)
			{
				Line += " (split pot)";
			}
			else if (CountPlayersInHand() == 1)
			{
				Line += " (everyone else folded)";
			}
			Renderer.PrintLine(Line);
		}
	}
}

void FTable::ReportEliminations()
{
	for (const std::unique_ptr<FPlayer>& Player : Players)
	{
		if (Player->Chips <= 0 && !Player->bIsEliminated)
		{
			Renderer.PrintLine("  " + Player->Name + " is out of chips and leaves the table.");
			Player->bIsEliminated = true;
		}
	}
}

// ---------------------------------------------------------------- Helpers

int32 FTable::CountActivePlayers() const
{
	return static_cast<int32>(std::count_if(Players.begin(), Players.end(),
		[](const std::unique_ptr<FPlayer>& Player) { return Player->Chips > 0; }));
}

int32 FTable::CountPlayersInHand() const
{
	return static_cast<int32>(std::count_if(Players.begin(), Players.end(),
		[](const std::unique_ptr<FPlayer>& Player) { return !Player->bIsEliminated && !Player->bIsFolded; }));
}

int32 FTable::CountPlayersAbleToAct() const
{
	return static_cast<int32>(std::count_if(Players.begin(), Players.end(),
		[](const std::unique_ptr<FPlayer>& Player) { return !Player->bIsEliminated && !Player->bIsFolded && !Player->bIsAllIn; }));
}

int32 FTable::NextSeat(int32 From, bool bRequireInHand) const
{
	const int32 NumSeats = static_cast<int32>(Players.size());
	for (int32 Offset = 1; Offset <= NumSeats; ++Offset)
	{
		const int32 Seat = (From + Offset) % NumSeats;
		const FPlayer& Player = *Players[Seat];
		if (Player.bIsEliminated || (bRequireInHand && Player.bIsFolded))
		{
			continue;
		}
		return Seat;
	}
	return INDEX_NONE;
}

void FTable::LogEvent(const FString& Message)
{
	RecentActions.push_back(Message);
	if (RecentActions.size() > MaxRecentActions)
	{
		RecentActions.erase(RecentActions.begin());
	}
	bTableDirty = true;
	Renderer.PrintLine("  > " + Message);
}

void FTable::ShowTable()
{
	Renderer.ShowTable(*this);
	bTableDirty = false;
}
