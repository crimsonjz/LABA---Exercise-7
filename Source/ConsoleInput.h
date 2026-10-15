#pragma once

#include "CoreTypes.h"

/** Defensive console input helpers: nothing here throws or trusts the user's text. */
class FConsoleInput
{
public:
	/** Prompts and reads one line. Returns false on end-of-input (stream closed). */
	static bool ReadLine(const FString& Prompt, FString& OutLine);

	/**
	 * Prompts until a whole number in [MinValue, MaxValue] is entered. An empty line returns DefaultValue
	 * when DefaultValue != INDEX_NONE. Returns false on end-of-input.
	 */
	static bool ReadInt32InRange(const FString& Prompt, int32 MinValue, int32 MaxValue, int32 DefaultValue, int32& OutValue);

	/** Strict parse: optional sign, digits only, no overflow, no trailing garbage. */
	static bool TryParseInt32(const FString& Text, int32& OutValue);

	static FString Trim(const FString& Text);
	static FString ToLower(const FString& Text);
};
