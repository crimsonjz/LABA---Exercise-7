#pragma once

#include "CoreTypes.h"

class FTable;

/** All console output of the table view lives here. */
class FConsoleRenderer
{
public:
	void PrintBanner(const FString& Title) const;
	void PrintLine(const FString& Text) const;

	/** Board, pot, seats (dealer/blinds/bets/status), the human's cards and the latest actions. */
	void ShowTable(const FTable& Table) const;
};
