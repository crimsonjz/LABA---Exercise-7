#pragma once

#include <memory>

#include "ConsoleRenderer.h"
#include "CoreTypes.h"
#include "Deck.h"
#include "GameTypes.h"
#include "Player.h"
#include "PotManager.h"

struct FTableConfig
{
	int32 StartingChips = 1000;
	int32 SmallBlind = 10;
	int32 BigBlind = 20;
};

/** Runs a No-Limit Texas Hold'em game: seating, blinds, betting rounds, showdown and payouts. */
class FTable
{
public:
	explicit FTable(const FTableConfig& InConfig);

	void AddPlayer(std::unique_ptr<FPlayer> Player);

	/** Plays hands until one player has all the chips or the human leaves / busts. */
	void Run();

	// Read-only state for the renderer.
	const TArray<std::unique_ptr<FPlayer>>& GetPlayers() const { return Players; }
	const TArray<FCard>& GetCommunityCards() const { return CommunityCards; }
	const TArray<FString>& GetRecentActions() const { return RecentActions; }
	const FTableConfig& GetConfig() const { return Config; }
	EGamePhase GetPhase() const { return Phase; }
	int32 GetHandNumber() const { return HandNumber; }
	int32 GetDealerIndex() const { return DealerIndex; }
	int32 GetSmallBlindIndex() const { return SmallBlindIndex; }
	int32 GetBigBlindIndex() const { return BigBlindIndex; }
	int32 GetCurrentActorIndex() const { return CurrentActorIndex; }
	int32 GetTotalPot() const { return Pot.GetTotalPot(); }
	int32 GetPlayerCurrentBet(int32 Seat) const { return Pot.GetCurrentBet(Seat); }
	bool ShouldRevealAllHands() const { return bRevealAllHands; }

private:
	void PlayHand();
	void StartHand();
	void PostBlind(int32 Seat, int32 Amount, const char* Label);
	void DealCommunityCards(EGamePhase NewPhase, int32 NumCards);
	void RunBettingRound(int32 FirstSeat, bool bPreFlop);
	void ResolveHand();
	void AwardPots(const TArray<FHandEvaluationResult>& Results);

	FActionContext BuildContext(int32 Seat) const;
	void ApplyDecision(int32 Seat, const FPlayerDecision& Decision);
	void CommitChips(int32 Seat, int32 Amount);
	void RaiseTo(int32 Seat, int32 TotalBet);

	bool NeedsToAct(int32 Seat) const;
	int32 CountActivePlayers() const;
	int32 CountPlayersInHand() const;
	int32 CountPlayersAbleToAct() const;
	int32 NextSeat(int32 From, bool bRequireInHand) const;

	void LogEvent(const FString& Message);
	void ShowTable();
	void ReportEliminations();

	FTableConfig Config;
	FConsoleRenderer Renderer;
	TArray<std::unique_ptr<FPlayer>> Players;
	FDeck Deck;
	FPotManager Pot;
	TArray<FCard> CommunityCards;
	TArray<FString> RecentActions;

	EGamePhase Phase = EGamePhase::PreFlop;
	int32 HandNumber = 0;
	int32 DealerIndex = INDEX_NONE;
	int32 SmallBlindIndex = INDEX_NONE;
	int32 BigBlindIndex = INDEX_NONE;
	int32 CurrentActorIndex = INDEX_NONE;
	int32 MinRaiseSize = 0;

	bool bRevealAllHands = false;
	bool bQuitRequested = false;
	bool bTableDirty = true;
};
