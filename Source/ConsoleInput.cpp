#include "ConsoleInput.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <iostream>

FString FConsoleInput::Trim(const FString& Text)
{
	const auto IsSpace = [](unsigned char Character) { return std::isspace(Character) != 0; };
	const auto First = std::find_if_not(Text.begin(), Text.end(), IsSpace);
	const auto Last = std::find_if_not(Text.rbegin(), Text.rend(), IsSpace).base();
	return (First < Last) ? FString(First, Last) : FString();
}

FString FConsoleInput::ToLower(const FString& Text)
{
	FString Result = Text;
	std::transform(Result.begin(), Result.end(), Result.begin(),
		[](unsigned char Character) { return static_cast<char>(std::tolower(Character)); });
	return Result;
}

bool FConsoleInput::ReadLine(const FString& Prompt, FString& OutLine)
{
	std::cout << Prompt << std::flush;
	if (!std::getline(std::cin, OutLine))
	{
		return false;
	}
	OutLine = Trim(OutLine);
	return true;
}

bool FConsoleInput::TryParseInt32(const FString& Text, int32& OutValue)
{
	FString Trimmed = Trim(Text);
	if (!Trimmed.empty() && Trimmed[0] == '+')
	{
		Trimmed.erase(0, 1);
	}
	if (Trimmed.empty())
	{
		return false;
	}

	int32 Parsed = 0;
	const char* Begin = Trimmed.data();
	const char* End = Begin + Trimmed.size();
	const std::from_chars_result ParseResult = std::from_chars(Begin, End, Parsed);
	if (ParseResult.ec != std::errc() || ParseResult.ptr != End)
	{
		return false;
	}

	OutValue = Parsed;
	return true;
}

bool FConsoleInput::ReadInt32InRange(const FString& Prompt, int32 MinValue, int32 MaxValue, int32 DefaultValue, int32& OutValue)
{
	while (true)
	{
		FString Line;
		if (!ReadLine(Prompt, Line))
		{
			return false;
		}

		if (Line.empty() && DefaultValue != INDEX_NONE)
		{
			OutValue = DefaultValue;
			return true;
		}

		int32 Parsed = 0;
		if (!TryParseInt32(Line, Parsed))
		{
			std::cout << "  '" << Line << "' is not a valid whole number.\n";
			continue;
		}
		if (Parsed < MinValue || Parsed > MaxValue)
		{
			std::cout << "  Please enter a number between " << MinValue << " and " << MaxValue << ".\n";
			continue;
		}

		OutValue = Parsed;
		return true;
	}
}
