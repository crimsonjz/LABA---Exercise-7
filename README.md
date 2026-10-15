# Texas Hold'em (No-Limit) - console, C++20, Unreal Engine coding standard

Build & run (CMake):

    cmake -S . -B build
    cmake --build build --config Release
    build/Release/TexasHoldemPoker      # game
    build/Release/PokerTests            # unit tests (also: ctest -C Release)

In game: `f` fold, `c` check/call, `r <to amount>` / `b <amount>` bet or raise, `a` all-in, `q` quit.

Layout: `Source/` (Card, Deck, HandEvaluator, PotManager, GameTypes, BotStrategy, Player, Table,
ConsoleRenderer, ConsoleInput, Main), `Tests/` (self-contained test framework + tests).
