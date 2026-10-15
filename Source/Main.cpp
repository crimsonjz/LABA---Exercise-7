#include <algorithm>
#include <iostream>
#include <memory>

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "BotStrategy.h"
#include "ConsoleInput.h"
#include "CoreTypes.h"
#include "Player.h"
#include "Table.h"

int main()
{
#ifdef _WIN32
	SetConsoleOutputCP(CP_UTF8); // Suit symbols
#endif

	std::cout << "=== Texas Hold'em Poker (No-Limit) ===\n"
			  << "Press Enter to accept the default shown in brackets.\n\n";

	FString HumanName;
	if (!FConsoleInput::ReadLine("Your name [You]: ", HumanName))
	{
		return 0;
	}
	if (HumanName.empty())
	{
		HumanName = "You";
	}
	if (HumanName.size() > 14)
	{
		HumanName.resize(14);
	}

	int32 NumBots = 0;
	int32 StartingChips = 0;
	int32 BigBlind = 0;
	if (!FConsoleInput::ReadInt32InRange("Number of AI opponents (1-8) [3]: ", 1, 8, 3, NumBots)
		|| !FConsoleInput::ReadInt32InRange("Starting chips (100-1000000) [1000]: ", 100, 1000000, 1000, StartingChips)
		|| !FConsoleInput::ReadInt32InRange("Big blind (2-" + std::to_string(StartingChips / 10) + ") [" + std::to_string(std::max(2, StartingChips / 50)) + "]: ",
			2, std::max(2, StartingChips / 10), std::max(2, StartingChips / 50), BigBlind))
	{
		return 0;
	}

	FTableConfig Config;
	Config.StartingChips = StartingChips;
	Config.BigBlind = BigBlind;
	Config.SmallBlind = std::max(1, BigBlind / 2);

	FTable Table(Config);
	Table.AddPlayer(std::make_unique<FHumanPlayer>(HumanName, StartingChips));

	const char* const BotNames[] = {"Alice", "Bob", "Charlie", "Dana", "Eve", "Frank", "Grace", "Heidi"};
	const float BotAggression[] = {0.35f, 0.6f, 0.5f, 0.75f, 0.4f, 0.65f, 0.3f, 0.55f};
	for (int32 Index = 0; Index < NumBots; ++Index)
	{
		Table.AddPlayer(std::make_unique<FAIPlayer>(
			BotNames[Index], StartingChips, std::make_unique<FBasicBotStrategy>(BotAggression[Index])));
	}

	Table.Run();
	return 0;
}
