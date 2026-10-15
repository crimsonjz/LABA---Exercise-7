#pragma once

#include <random>

#include "Card.h"
#include "CoreTypes.h"

class FDeck
{
public:
	/** Fresh ordered 52-card deck, engine seeded from std::random_device. */
	FDeck();

	/** Deterministic variant (used by tests). */
	explicit FDeck(uint32 Seed);

	/** Restores all 52 cards in canonical order. */
	void Reset();

	void Shuffle();

	/** Removes and returns the top card. Must not be called on an empty deck. */
	FCard Draw();

	int32 Remaining() const { return static_cast<int32>(Cards.size()); }
	bool IsEmpty() const { return Cards.empty(); }

private:
	TArray<FCard> Cards;
	std::mt19937 RandomEngine;
};
