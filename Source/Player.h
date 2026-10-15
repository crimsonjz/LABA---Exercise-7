#pragma once

#include <memory>

#include "BotStrategy.h"
#include "Card.h"
#include "CoreTypes.h"
#include "GameTypes.h"

/** Base model of a seated player. Subclasses decide how actions are chosen. */
class FPlayer
{
public:
	FPlayer(FString InName, int32 InChips);
	virtual ~FPlayer() = default;

	virtual FPlayerDecision DecideAction(const FActionContext& Context) = 0;
	virtual bool IsHuman() const { return false; }
	virtual bool WantsToQuit() const { return false; }

	/** Clears per-hand state; a player without chips is marked eliminated. */
	void ResetForNewHand();

	/** Removes up to Amount chips and returns how many were actually taken. */
	int32 RemoveChips(int32 Amount);
	void AddChips(int32 Amount);

	FString Name;
	int32 Chips = 0;
	TArray<FCard> Hand;

	bool bIsFolded = false;
	bool bIsAllIn = false;
	bool bHasActed = false;
	bool bIsEliminated = false;

	/** True when only an incomplete (short all-in) raise happened since this player acted. */
	bool bIsRaiseClosed = false;
};

class FHumanPlayer : public FPlayer
{
public:
	FHumanPlayer(FString InName, int32 InChips);

	virtual FPlayerDecision DecideAction(const FActionContext& Context) override;
	virtual bool IsHuman() const override { return true; }
	virtual bool WantsToQuit() const override { return bQuitRequested; }

	/**
	 * Parses a command line such as "f", "c", "r 120", "allin". On success OutDecision.Amount may be
	 * INDEX_NONE for bet/raise commands without an amount. "quit" sets bOutQuit.
	 */
	static bool TryParseCommand(const FString& Line, const FActionContext& Context,
		FPlayerDecision& OutDecision, FString& OutError, bool& bOutQuit);

private:
	static FString BuildOptionsText(const FActionContext& Context);

	bool bQuitRequested = false;
};

class FAIPlayer : public FPlayer
{
public:
	FAIPlayer(FString InName, int32 InChips, std::unique_ptr<IBotStrategy> InStrategy);

	virtual FPlayerDecision DecideAction(const FActionContext& Context) override;

private:
	std::unique_ptr<IBotStrategy> Strategy;
};
