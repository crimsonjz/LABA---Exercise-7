#include "ConsoleRenderer.h"

#include <iomanip>
#include <iostream>
#include <sstream>

#include "HandEvaluator.h"
#include "Table.h"

namespace
{
	FString CardToBox(const FCard& Card)
	{
		return "[" + Card.ToDisplayString() + "]";
	}

	FString FormatBoard(const TArray<FCard>& Community)
	{
		FString Text;
		for (size_t Index = 0; Index < 5; ++Index)
		{
			Text += (Index < Community.size()) ? CardToBox(Community[Index]) : FString("[??]");
			Text += ' ';
		}
		return Text;
	}

	FString FormatHand(const FPlayer& Player, bool bRevealed)
	{
		if (Player.Hand.empty())
		{
			return "";
		}
		FString Text;
		for (const FCard& Card : Player.Hand)
		{
			Text += bRevealed ? CardToBox(Card) : FString("[??]");
		}
		return Text;
	}
}

void FConsoleRenderer::PrintBanner(const FString& Title) const
{
	std::cout << "\n" << FString(64, '=') << "\n  " << Title << "\n" << FString(64, '=') << "\n";
}

void FConsoleRenderer::PrintLine(const FString& Text) const
{
	std::cout << Text << "\n";
}

void FConsoleRenderer::ShowTable(const FTable& Table) const
{
	const TArray<std::unique_ptr<FPlayer>>& Players = Table.GetPlayers();
	const FTableConfig& Config = Table.GetConfig();

	std::cout << "\n" << FString(64, '-') << "\n";
	std::cout << "  Hand #" << Table.GetHandNumber() << "   Blinds " << Config.SmallBlind << "/" << Config.BigBlind
			  << "   Phase: " << GamePhaseToString(Table.GetPhase()) << "\n";
	std::cout << "  Board: " << FormatBoard(Table.GetCommunityCards()) << "\n";
	std::cout << "  Pot:   " << Table.GetTotalPot() << "\n";
	std::cout << FString(64, '-') << "\n";
	std::cout << "   " << std::left << std::setw(5) << "Pos" << std::setw(16) << "Player" << std::right
			  << std::setw(8) << "Chips" << std::setw(8) << "Bet" << "  Status\n";

	const FPlayer* HumanPlayer = nullptr;
	for (size_t Index = 0; Index < Players.size(); ++Index)
	{
		const FPlayer& Player = *Players[Index];
		const int32 Seat = static_cast<int32>(Index);
		if (Player.IsHuman())
		{
			HumanPlayer = &Player;
		}

		FString Position;
		if (Seat == Table.GetDealerIndex())     { Position += "D "; }
		if (Seat == Table.GetSmallBlindIndex()) { Position += "SB"; }
		if (Seat == Table.GetBigBlindIndex())   { Position += "BB"; }

		FString Status;
		if (Player.bIsEliminated)   { Status = "out"; }
		else if (Player.bIsFolded)  { Status = "folded"; }
		else if (Player.bIsAllIn)   { Status = "ALL-IN"; }
		else if (Seat == Table.GetCurrentActorIndex()) { Status = "<- to act"; }

		if (!Player.bIsEliminated && !Player.bIsFolded && Table.ShouldRevealAllHands())
		{
			Status += " " + FormatHand(Player, true);
		}

		std::cout << (Seat == Table.GetCurrentActorIndex() ? " > " : "   ") << std::left << std::setw(5) << Position
				  << std::setw(16) << Player.Name << std::right << std::setw(8) << Player.Chips
				  << std::setw(8) << Table.GetPlayerCurrentBet(Seat) << "  " << Status << "\n";
	}

	if (HumanPlayer && !HumanPlayer->Hand.empty() && !HumanPlayer->bIsEliminated)
	{
		std::cout << FString(64, '-') << "\n  Your cards: " << FormatHand(*HumanPlayer, true);

		TArray<FCard> AllCards = HumanPlayer->Hand;
		AllCards.insert(AllCards.end(), Table.GetCommunityCards().begin(), Table.GetCommunityCards().end());
		if (AllCards.size() >= 5)
		{
			std::cout << "   (" << FHandEvaluator::Evaluate(AllCards).ToString() << ")";
		}
		std::cout << "\n";
	}

	if (!Table.GetRecentActions().empty())
	{
		std::cout << FString(64, '-') << "\n  Recent actions:\n";
		for (const FString& Action : Table.GetRecentActions())
		{
			std::cout << "    " << Action << "\n";
		}
	}
	std::cout << FString(64, '-') << "\n";
}
