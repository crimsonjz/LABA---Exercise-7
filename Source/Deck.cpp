#include "Deck.h"

#include <algorithm>
#include <cassert>

FDeck::FDeck() : RandomEngine(std::random_device{}())
{
	Reset();
}

FDeck::FDeck(uint32 Seed) : RandomEngine(Seed)
{
	Reset();
}

void FDeck::Reset()
{
	Cards.clear();
	Cards.reserve(52);
	for (uint8 SuitValue = 0; SuitValue < 4; ++SuitValue)
	{
		for (uint8 RankValue = 2; RankValue <= 14; ++RankValue)
		{
			Cards.emplace_back(static_cast<ECardRank>(RankValue), static_cast<ECardSuit>(SuitValue));
		}
	}
}

void FDeck::Shuffle()
{
	std::shuffle(Cards.begin(), Cards.end(), RandomEngine);
}

FCard FDeck::Draw()
{
	assert(!Cards.empty() && "Draw called on an empty deck");
	if (Cards.empty())
	{
		return FCard();
	}

	const FCard TopCard = Cards.back();
	Cards.pop_back();
	return TopCard;
}
