#include "Card.h"

#include <cctype>
#include <sstream>

namespace
{
	constexpr char RankChars[] = "23456789TJQKA";

	char RankToChar(ECardRank Rank)
	{
		return RankChars[static_cast<uint8>(Rank) - 2];
	}

	char SuitToChar(ECardSuit Suit)
	{
		switch (Suit)
		{
		case ECardSuit::Clubs:    return 'c';
		case ECardSuit::Diamonds: return 'd';
		case ECardSuit::Hearts:   return 'h';
		case ECardSuit::Spades:   return 's';
		}
		return '?';
	}

	const char* SuitToSymbol(ECardSuit Suit)
	{
		switch (Suit)
		{
		case ECardSuit::Clubs:    return "♣";
		case ECardSuit::Diamonds: return "♦";
		case ECardSuit::Hearts:   return "♥";
		case ECardSuit::Spades:   return "♠";
		}
		return "?";
	}
}

FString FCard::ToString() const
{
	FString Result;
	Result += RankToChar(Rank);
	Result += SuitToChar(Suit);
	return Result;
}

FString FCard::ToDisplayString() const
{
	FString Result;
	Result += RankToChar(Rank);
	Result += SuitToSymbol(Suit);
	return Result;
}

bool FCard::TryParse(const FString& Text, FCard& OutCard)
{
	FString Normalized;
	for (const char Character : Text)
	{
		Normalized += static_cast<char>(std::tolower(static_cast<unsigned char>(Character)));
	}

	if (Normalized.rfind("10", 0) == 0)
	{
		Normalized = "t" + Normalized.substr(2);
	}

	if (Normalized.size() != 2)
	{
		return false;
	}

	const char RankChar = static_cast<char>(std::toupper(static_cast<unsigned char>(Normalized[0])));
	const char SuitChar = Normalized[1];

	int32 RankIndex = INDEX_NONE;
	for (int32 Index = 0; RankChars[Index] != '\0'; ++Index)
	{
		if (RankChars[Index] == RankChar)
		{
			RankIndex = Index;
			break;
		}
	}
	if (RankIndex == INDEX_NONE)
	{
		return false;
	}

	ECardSuit ParsedSuit;
	switch (SuitChar)
	{
	case 'c': ParsedSuit = ECardSuit::Clubs;    break;
	case 'd': ParsedSuit = ECardSuit::Diamonds; break;
	case 'h': ParsedSuit = ECardSuit::Hearts;   break;
	case 's': ParsedSuit = ECardSuit::Spades;   break;
	default:  return false;
	}

	OutCard = FCard(static_cast<ECardRank>(RankIndex + 2), ParsedSuit);
	return true;
}

TArray<FCard> FCard::ParseMany(const FString& Text)
{
	TArray<FCard> Cards;
	std::istringstream Stream(Text);
	FString Token;
	while (Stream >> Token)
	{
		FCard Card;
		if (TryParse(Token, Card))
		{
			Cards.push_back(Card);
		}
	}
	return Cards;
}
