#include <set>

#include "ConsoleInput.h"
#include "Deck.h"
#include "GameTypes.h"
#include "Player.h"
#include "PotManager.h"
#include "TestFramework.h"

namespace
{
	FHandEvaluationResult Eval(const char* Cards)
	{
		return FHandEvaluator::Evaluate(FCard::ParseMany(Cards));
	}

	int32 Sum(const TArray<FPotAward>& Awards, int32 Seat)
	{
		int32 Total = 0;
		for (const FPotAward& Award : Awards)
		{
			for (size_t Index = 0; Index < Award.Winners.size(); ++Index)
			{
				if (Award.Winners[Index] == Seat)
				{
					Total += Award.Payouts[Index];
				}
			}
		}
		return Total;
	}

	FActionContext MakeFacingBet()
	{
		FActionContext Context;
		Context.PlayerChips = 500;
		Context.PlayerCurrentBet = 20;
		Context.HighestBet = 100;
		Context.ToCall = 80;
		Context.MinRaiseTo = 180;
		Context.MaxBetTo = 520;
		Context.BigBlind = 20;
		return Context;
	}
}

// ---------------------------------------------------------------- Deck

TEST_CASE(Deck_Has52UniqueCards)
{
	FDeck Deck(42);
	Deck.Shuffle();
	CHECK_EQ(Deck.Remaining(), 52);

	std::set<FString> Seen;
	while (!Deck.IsEmpty())
	{
		Seen.insert(Deck.Draw().ToString());
	}
	CHECK_EQ(Seen.size(), static_cast<size_t>(52));
}

TEST_CASE(Deck_SameSeedSameShuffle)
{
	FDeck First(7);
	FDeck Second(7);
	First.Shuffle();
	Second.Shuffle();
	CHECK(First.Draw() == Second.Draw());
	CHECK(First.Draw() == Second.Draw());
}

// ---------------------------------------------------------------- Pots

TEST_CASE(Pot_SimpleWinnerTakesAll)
{
	FPotManager Pot;
	Pot.Reset(3);
	Pot.PostBet(0, 100);
	Pot.PostBet(1, 100);
	Pot.PostBet(2, 100);
	Pot.MarkFolded(2);

	TArray<FHandEvaluationResult> Results(3);
	Results[0] = Eval("As Ah Kd 7c 2s");
	Results[1] = Eval("Qs Qh Kd 7c 2s");

	const TArray<FPotAward> Awards = Pot.AwardPots(Results, 0);
	CHECK_EQ(Awards.size(), static_cast<size_t>(1));
	CHECK_EQ(Sum(Awards, 0), 300);
	CHECK_EQ(Sum(Awards, 1), 0);
}

TEST_CASE(Pot_SidepotsWithTwoAllIns)
{
	// Seat 0 all-in for 100, seat 1 all-in for 300, seat 2 calls 300.
	FPotManager Pot;
	Pot.Reset(3);
	Pot.PostBet(0, 100);
	Pot.PostBet(1, 300);
	Pot.PostBet(2, 300);

	const TArray<FSidePot> Pots = Pot.BuildPots();
	CHECK_EQ(Pots.size(), static_cast<size_t>(2));
	CHECK_EQ(Pots[0].Amount, 300);
	CHECK_EQ(Pots[0].EligiblePlayers.size(), static_cast<size_t>(3));
	CHECK_EQ(Pots[1].Amount, 400);
	CHECK_EQ(Pots[1].EligiblePlayers.size(), static_cast<size_t>(2));

	// Short stack has the best hand, seat 1 second best.
	TArray<FHandEvaluationResult> Results(3);
	Results[0] = Eval("As Ah Kd 7c 2s");
	Results[1] = Eval("Ks Kh Qd 7c 2s");
	Results[2] = Eval("Js Jh Qd 7c 2s");
	const TArray<FPotAward> Awards = Pot.AwardPots(Results, 0);
	CHECK_EQ(Sum(Awards, 0), 300);
	CHECK_EQ(Sum(Awards, 1), 400);
	CHECK_EQ(Sum(Awards, 2), 0);
}

TEST_CASE(Pot_ThreeLevelsOfAllIn)
{
	FPotManager Pot;
	Pot.Reset(4);
	Pot.PostBet(0, 50);
	Pot.PostBet(1, 150);
	Pot.PostBet(2, 400);
	Pot.PostBet(3, 400);

	const TArray<FSidePot> Pots = Pot.BuildPots();
	CHECK_EQ(Pots.size(), static_cast<size_t>(3));
	CHECK_EQ(Pots[0].Amount, 200);
	CHECK_EQ(Pots[1].Amount, 300);
	CHECK_EQ(Pots[2].Amount, 500);
	CHECK_EQ(Pot.GetTotalPot(), 1000);
}

TEST_CASE(Pot_FoldedChipsStayInPotButFolderCannotWin)
{
	FPotManager Pot;
	Pot.Reset(3);
	Pot.PostBet(0, 50);  // Folds after betting
	Pot.PostBet(1, 100);
	Pot.PostBet(2, 100);
	Pot.MarkFolded(0);

	TArray<FHandEvaluationResult> Results(3);
	Results[0] = Eval("As Ah Ad Ac 2s"); // Would win, but folded
	Results[1] = Eval("Ks Kh Qd 7c 2s");
	Results[2] = Eval("Js Jh Qd 7c 2s");
	const TArray<FPotAward> Awards = Pot.AwardPots(Results, 0);
	CHECK_EQ(Sum(Awards, 0), 0);
	CHECK_EQ(Sum(Awards, 1), 250);
}

TEST_CASE(Pot_UncalledBetIsReturned)
{
	FPotManager Pot;
	Pot.Reset(2);
	Pot.PostBet(0, 500);
	Pot.PostBet(1, 200); // All-in for less

	TArray<FHandEvaluationResult> Results(2);
	Results[0] = Eval("Qs Qh Kd 7c 2s");
	Results[1] = Eval("As Ah Kd 7c 2s");
	const TArray<FPotAward> Awards = Pot.AwardPots(Results, 0);
	CHECK_EQ(Sum(Awards, 1), 400); // Wins the matched part
	CHECK_EQ(Sum(Awards, 0), 300); // Gets the uncalled 300 back
}

TEST_CASE(Pot_SplitPotOddChipGoesLeftOfDealer)
{
	FPotManager Pot;
	Pot.Reset(3);
	Pot.PostBet(0, 50);
	Pot.PostBet(1, 51);
	Pot.PostBet(2, 50);

	TArray<FHandEvaluationResult> Results(3);
	Results[0] = Eval("As Ah Kd 7c 2s");
	Results[1] = Eval("Ad Ac Kh 7d 2d");
	Results[2] = Eval("Js Jh Qd 7c 2s");
	// Pots: 150 (seats 0,1,2) and 1 (seat 1 only).
	const TArray<FPotAward> Awards = Pot.AwardPots(Results, 0);
	CHECK_EQ(Sum(Awards, 0), 75);
	CHECK_EQ(Sum(Awards, 1), 76);
	CHECK_EQ(Sum(Awards, 2), 0);

	FPotManager OddPot;
	OddPot.Reset(3);
	OddPot.PostBet(0, 33);
	OddPot.PostBet(1, 33);
	OddPot.PostBet(2, 33);
	const TArray<FPotAward> OddAwards = OddPot.AwardPots(Results, 0);
	// Seat 0 and 1 split 99: 50 + 49, odd chip goes to seat 1 (first left of the dealer at seat 0).
	CHECK_EQ(Sum(OddAwards, 1), 50);
	CHECK_EQ(Sum(OddAwards, 0), 49);
}

TEST_CASE(Pot_RoundBetsResetButContributionsStay)
{
	FPotManager Pot;
	Pot.Reset(2);
	Pot.PostBet(0, 40);
	Pot.PostBet(1, 40);
	CHECK_EQ(Pot.GetHighestBet(), 40);
	Pot.StartNewRound();
	CHECK_EQ(Pot.GetHighestBet(), 0);
	CHECK_EQ(Pot.GetCurrentBet(0), 0);
	CHECK_EQ(Pot.GetTotalPot(), 80);
	CHECK_EQ(Pot.GetTotalContribution(1), 40);
}

// ---------------------------------------------------------------- Input & validation

TEST_CASE(Input_ParseInt32IsStrict)
{
	int32 Value = 0;
	CHECK(FConsoleInput::TryParseInt32("123", Value));
	CHECK_EQ(Value, 123);
	CHECK(FConsoleInput::TryParseInt32("  +45 ", Value));
	CHECK_EQ(Value, 45);
	CHECK(FConsoleInput::TryParseInt32("-7", Value));
	CHECK_EQ(Value, -7);
	CHECK(!FConsoleInput::TryParseInt32("", Value));
	CHECK(!FConsoleInput::TryParseInt32("abc", Value));
	CHECK(!FConsoleInput::TryParseInt32("12abc", Value));
	CHECK(!FConsoleInput::TryParseInt32("1.5", Value));
	CHECK(!FConsoleInput::TryParseInt32("99999999999999999999", Value));
}

TEST_CASE(Human_CommandParsing)
{
	const FActionContext Context = MakeFacingBet();
	FPlayerDecision Decision;
	FString Error;
	bool bQuit = false;

	CHECK(FHumanPlayer::TryParseCommand("r 200", Context, Decision, Error, bQuit));
	CHECK(Decision.Action == EPlayerAction::Raise);
	CHECK_EQ(Decision.Amount, 200);

	CHECK(FHumanPlayer::TryParseCommand("  C ", Context, Decision, Error, bQuit));
	CHECK(Decision.Action == EPlayerAction::Call);

	CHECK(FHumanPlayer::TryParseCommand("raise", Context, Decision, Error, bQuit));
	CHECK_EQ(Decision.Amount, INDEX_NONE); // Amount will be prompted for

	CHECK(!FHumanPlayer::TryParseCommand("r abc", Context, Decision, Error, bQuit));
	CHECK(!FHumanPlayer::TryParseCommand("r -50", Context, Decision, Error, bQuit));
	CHECK(!FHumanPlayer::TryParseCommand("r 99999999999999", Context, Decision, Error, bQuit));
	CHECK(!FHumanPlayer::TryParseCommand("hello", Context, Decision, Error, bQuit));
	CHECK(!FHumanPlayer::TryParseCommand("", Context, Decision, Error, bQuit));
	CHECK(!FHumanPlayer::TryParseCommand("f 10", Context, Decision, Error, bQuit));

	CHECK(FHumanPlayer::TryParseCommand("q", Context, Decision, Error, bQuit));
	CHECK(bQuit);
}

TEST_CASE(Validation_CheckCallFold)
{
	FActionContext Context = MakeFacingBet();
	FString Error;
	CHECK(Context.Validate(FPlayerDecision(EPlayerAction::Fold), Error));
	CHECK(Context.Validate(FPlayerDecision(EPlayerAction::Call), Error));
	CHECK(!Context.Validate(FPlayerDecision(EPlayerAction::Check), Error));

	Context.ToCall = 0;
	Context.HighestBet = Context.PlayerCurrentBet;
	CHECK(Context.Validate(FPlayerDecision(EPlayerAction::Check), Error));
	CHECK(!Context.Validate(FPlayerDecision(EPlayerAction::Call), Error));
}

TEST_CASE(Validation_RaiseLimits)
{
	const FActionContext Context = MakeFacingBet();
	FString Error;
	CHECK(!Context.Validate(FPlayerDecision(EPlayerAction::Raise, 150), Error)); // Below min raise
	CHECK(Context.Validate(FPlayerDecision(EPlayerAction::Raise, 180), Error));
	CHECK(Context.Validate(FPlayerDecision(EPlayerAction::Raise, 520), Error));  // Whole stack
	CHECK(!Context.Validate(FPlayerDecision(EPlayerAction::Raise, 521), Error)); // More than the stack
	CHECK(!Context.Validate(FPlayerDecision(EPlayerAction::Raise, -10), Error));
	CHECK(!Context.Validate(FPlayerDecision(EPlayerAction::Raise, 0), Error));
	CHECK(!Context.Validate(FPlayerDecision(EPlayerAction::Bet, 100), Error));   // A bet already exists
	CHECK(Context.Validate(FPlayerDecision(EPlayerAction::AllIn), Error));
}

TEST_CASE(Validation_BetLimitsAndShortAllIn)
{
	FActionContext Context;
	Context.PlayerChips = 15;
	Context.MaxBetTo = 15;
	Context.MinRaiseTo = 20;
	Context.BigBlind = 20;
	FString Error;
	CHECK(!Context.Validate(FPlayerDecision(EPlayerAction::Bet, 10), Error)); // Under the big blind, not all-in
	CHECK(Context.Validate(FPlayerDecision(EPlayerAction::Bet, 15), Error));   // Short all-in is fine
	CHECK(!Context.Validate(FPlayerDecision(EPlayerAction::Bet, 16), Error));  // More than the stack
	CHECK(!Context.Validate(FPlayerDecision(EPlayerAction::Bet, -1), Error));
	CHECK(!Context.Validate(FPlayerDecision(EPlayerAction::Raise, 20), Error)); // Nothing to raise yet
}

TEST_CASE(Validation_ClosedBettingForbidsRaise)
{
	FActionContext Context = MakeFacingBet();
	Context.bCanRaise = false;
	FString Error;
	CHECK(!Context.Validate(FPlayerDecision(EPlayerAction::Raise, 300), Error));
	CHECK(!Context.Validate(FPlayerDecision(EPlayerAction::AllIn), Error));
	CHECK(Context.Validate(FPlayerDecision(EPlayerAction::Call), Error));
}
