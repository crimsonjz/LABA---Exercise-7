#pragma once

#include "CoreTypes.h"

enum class ECardSuit : uint8
{
	Clubs,
	Diamonds,
	Hearts,
	Spades
};

enum class ECardRank : uint8
{
	Two = 2,
	Three,
	Four,
	Five,
	Six,
	Seven,
	Eight,
	Nine,
	Ten,
	Jack,
	Queen,
	King,
	Ace
};

struct FCard
{
	ECardRank Rank = ECardRank::Two;
	ECardSuit Suit = ECardSuit::Clubs;

	FCard() = default;
	FCard(ECardRank InRank, ECardSuit InSuit) : Rank(InRank), Suit(InSuit) {}

	uint8 GetRankValue() const { return static_cast<uint8>(Rank); }

	/** ASCII form, e.g. "As", "Td", "2c". */
	FString ToString() const;

	/** Console form with suit symbol, e.g. "A♠". */
	FString ToDisplayString() const;

	/** Parses "As", "td", "10h". Case insensitive. */
	static bool TryParse(const FString& Text, FCard& OutCard);

	/** Parses a whitespace separated list ("Ah Kd 7c"). Invalid tokens are skipped. */
	static TArray<FCard> ParseMany(const FString& Text);

	bool operator==(const FCard& Other) const { return Rank == Other.Rank && Suit == Other.Suit; }
	bool operator!=(const FCard& Other) const { return !(*this == Other); }
};
