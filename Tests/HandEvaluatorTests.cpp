#include "HandEvaluator.h"
#include "TestFramework.h"

namespace
{
	FHandEvaluationResult Eval(const char* Cards)
	{
		return FHandEvaluator::Evaluate(FCard::ParseMany(Cards));
	}

	bool IsRank(const FHandEvaluationResult& Result, EHandRank Expected)
	{
		return Result.HandRank == Expected;
	}
}

TEST_CASE(HandRanks_AllTenCategories)
{
	CHECK(IsRank(Eval("Ah Kh Qh Jh Th"), EHandRank::RoyalFlush));
	CHECK(IsRank(Eval("9s 8s 7s 6s 5s"), EHandRank::StraightFlush));
	CHECK(IsRank(Eval("9s 9h 9d 9c 2s"), EHandRank::FourOfAKind));
	CHECK(IsRank(Eval("Ks Kh Kd 4c 4s"), EHandRank::FullHouse));
	CHECK(IsRank(Eval("As 9s 7s 4s 2s"), EHandRank::Flush));
	CHECK(IsRank(Eval("9s 8h 7d 6c 5s"), EHandRank::Straight));
	CHECK(IsRank(Eval("7s 7h 7d Kc 2s"), EHandRank::ThreeOfAKind));
	CHECK(IsRank(Eval("7s 7h 2d 2c Ks"), EHandRank::TwoPair));
	CHECK(IsRank(Eval("7s 7h 4d 2c Ks"), EHandRank::OnePair));
	CHECK(IsRank(Eval("As 9h 7d 4c 2s"), EHandRank::HighCard));
}

TEST_CASE(HandRanks_OrderingIsStrictlyIncreasing)
{
	const char* Hands[] = {
		"As 9h 7d 4c 2s", "7s 7h 4d 2c Ks", "7s 7h 2d 2c Ks", "7s 7h 7d Kc 2s", "9s 8h 7d 6c 5s",
		"As 9s 7s 4s 2s", "Ks Kh Kd 4c 4s", "9s 9h 9d 9c 2s", "9s 8s 7s 6s 5s", "Ah Kh Qh Jh Th"};
	for (size_t Index = 1; Index < std::size(Hands); ++Index)
	{
		CHECK(Eval(Hands[Index]).Compare(Eval(Hands[Index - 1])) > 0);
	}
}

TEST_CASE(Straight_WheelIsFiveHighAndLosesToSixHigh)
{
	const FHandEvaluationResult Wheel = Eval("Ah 2d 3c 4s 5h");
	CHECK(IsRank(Wheel, EHandRank::Straight));
	CHECK_EQ(static_cast<int32>(Wheel.Kickers[0]), 5);
	CHECK(Eval("2h 3d 4c 5s 6h").Compare(Wheel) > 0);
	CHECK(Eval("Th Jd Qc Ks Ah").Compare(Eval("9h Td Jc Qs Kh")) > 0);
}

TEST_CASE(Straight_DoesNotWrapAroundTheAce)
{
	CHECK(IsRank(Eval("Qh Kd Ac 2s 3h"), EHandRank::HighCard));
}

TEST_CASE(Flush_BeatsStraight)
{
	// Seven cards that contain both a heart flush and an 8-Q straight: the flush must win.
	const FHandEvaluationResult Result = Eval("9h 2h 8h Th 3h Jd Qc");
	CHECK(IsRank(Result, EHandRank::Flush));
	CHECK(Result.Compare(Eval("8s 9d Tc Jh Qd")) > 0);
}

TEST_CASE(Flush_ComparesCardByCard)
{
	CHECK(Eval("Ah 9h 7h 4h 3h").Compare(Eval("Ad 9d 7d 4d 2d")) > 0);
	CHECK(Eval("Kh Qh Jh 9h 2h").Compare(Eval("Ad 3d 4d 5d 7d")) < 0);
}

TEST_CASE(TwoPair_KickerDecides)
{
	const FHandEvaluationResult AceKicker = Eval("Ks Kd 7c 7h Ad");
	const FHandEvaluationResult QueenKicker = Eval("Kh Kc 7s 7d Qd");
	CHECK(AceKicker.Compare(QueenKicker) > 0);
	CHECK(QueenKicker.Compare(AceKicker) < 0);
}

TEST_CASE(TwoPair_EqualKickerIsATie)
{
	CHECK_EQ(Eval("Ks Kd 7c 7h Ad").Compare(Eval("Kh Kc 7s 7d As")), 0);
}

TEST_CASE(TwoPair_HigherPairWinsBeforeKicker)
{
	CHECK(Eval("Ks Kd 3c 3h 2d").Compare(Eval("Qs Qd Jc Jh Ad")) > 0);
	CHECK(Eval("Ks Kd 8c 8h 2d").Compare(Eval("Ks Kc 7s 7d Ad")) > 0);
}

TEST_CASE(OnePair_KickersAreComparedInOrder)
{
	CHECK(Eval("As Ah Kd 7c 2s").Compare(Eval("Ad Ac Qd Jc Ts")) > 0);
	CHECK(Eval("As Ah Kd 7c 3s").Compare(Eval("Ad Ac Kc 7d 2h")) > 0);
	CHECK_EQ(Eval("As Ah Kd 7c 2s").Compare(Eval("Ad Ac Kc 7d 2h")), 0);
}

TEST_CASE(HighCard_AllFiveCardsMatter)
{
	CHECK(Eval("As Kd 9c 5h 3d").Compare(Eval("Ac Kh 9d 5s 2c")) > 0);
}

TEST_CASE(FullHouse_TripsThenPair)
{
	CHECK(Eval("3s 3h 3d 2c 2s").Compare(Eval("2s 2h 2d Ac Ks")) > 0);
	CHECK(Eval("Ks Kh Kd 4c 4s").Compare(Eval("Qs Qh Qd Ac As")) > 0);
	CHECK(Eval("Ks Kh Kd 5c 5s").Compare(Eval("Kc Kh Kd 4c 4s")) > 0);
}

TEST_CASE(SevenCards_PicksBestFive)
{
	// Hole 2d 3d are irrelevant: aces full of kings is on the board.
	const FHandEvaluationResult Result = Eval("2d 3d Ah Ad As Kd Kh");
	CHECK(IsRank(Result, EHandRank::FullHouse));
	CHECK_EQ(static_cast<int32>(Result.Kickers[0]), 14);
	CHECK_EQ(static_cast<int32>(Result.Kickers[1]), 13);
	CHECK_EQ(Result.BestFive.size(), static_cast<size_t>(5));
}

TEST_CASE(SevenCards_ThreePairsUsesTwoHighestAndBestKicker)
{
	const FHandEvaluationResult Result = Eval("Ah Ad Kh Kd Qh Qd 2c");
	CHECK(IsRank(Result, EHandRank::TwoPair));
	CHECK_EQ(static_cast<int32>(Result.Kickers[0]), 14);
	CHECK_EQ(static_cast<int32>(Result.Kickers[1]), 13);
	CHECK_EQ(static_cast<int32>(Result.Kickers[2]), 12); // Queen kicker, not the deuce
}

TEST_CASE(SevenCards_TwoTripsMakeFullHouseWithHigherTrips)
{
	const FHandEvaluationResult Result = Eval("9h 9d 9s 4h 4d 4s Kc");
	CHECK(IsRank(Result, EHandRank::FullHouse));
	CHECK_EQ(static_cast<int32>(Result.Kickers[0]), 9);
	CHECK_EQ(static_cast<int32>(Result.Kickers[1]), 4);
}

TEST_CASE(SevenCards_StraightFlushBeatsQuads)
{
	const FHandEvaluationResult StraightFlush = Eval("5h 6h 7h 8h 9h 9d 9c");
	const FHandEvaluationResult Quads = Eval("9h 9d 9c 9s Ah Kd 2c");
	CHECK(IsRank(StraightFlush, EHandRank::StraightFlush));
	CHECK(IsRank(Quads, EHandRank::FourOfAKind));
	CHECK(StraightFlush.Compare(Quads) > 0);
}

TEST_CASE(SevenCards_StraightPicksHighestRun)
{
	const FHandEvaluationResult Result = Eval("3c 4d 5h 6s 7c 8d Kd");
	CHECK(IsRank(Result, EHandRank::Straight));
	CHECK_EQ(static_cast<int32>(Result.Kickers[0]), 8);
}

TEST_CASE(SevenCards_BoardPlaysIsATie)
{
	const FHandEvaluationResult PlayerOne = Eval("2c 3d Ah Kh Qh Jh Th");
	const FHandEvaluationResult PlayerTwo = Eval("4c 5d Ah Kh Qh Jh Th");
	CHECK(IsRank(PlayerOne, EHandRank::RoyalFlush));
	CHECK_EQ(PlayerOne.Compare(PlayerTwo), 0);
}

TEST_CASE(SevenCards_KickerFromHoleCardBreaksBoardTie)
{
	// Same board pair; the second player's ace kicker wins.
	const FHandEvaluationResult King = Eval("Kc 4d 9s 9h 2c 7d 3h");
	const FHandEvaluationResult Ace = Eval("Ac 5d 9s 9h 2c 7d 3h");
	CHECK(Ace.Compare(King) > 0);
}

TEST_CASE(Evaluate_FewerThanFiveCardsIsHarmless)
{
	const FHandEvaluationResult Result = Eval("Ah Kd");
	CHECK(IsRank(Result, EHandRank::HighCard));
	CHECK(Result.Kickers.empty());
}

TEST_CASE(Description_IsReadable)
{
	CHECK_EQ(Eval("Ks Kh Kd 4c 4s").ToString(), FString("Full House, Kings full of Fours"));
	CHECK_EQ(Eval("Ah Kh Qh Jh Th").ToString(), FString("Royal Flush"));
	CHECK_EQ(Eval("6s 6h 4d 2c Ks").ToString(), FString("Pair of Sixes"));
}

TEST_CASE(Card_ParsingAndFormatting)
{
	FCard Card;
	CHECK(FCard::TryParse("10h", Card));
	CHECK_EQ(Card.ToString(), FString("Th"));
	CHECK(FCard::TryParse("aS", Card));
	CHECK_EQ(Card.ToString(), FString("As"));
	CHECK(!FCard::TryParse("1h", Card));
	CHECK(!FCard::TryParse("Ax", Card));
	CHECK(!FCard::TryParse("", Card));
}
