#pragma once

#include <iostream>
#include <sstream>

#include "CoreTypes.h"

/** Minimal self-registering test framework (no external dependencies). */
struct FTestCase
{
	const char* Name;
	void (*Function)();
};

TArray<FTestCase>& GetTestRegistry();
void ReportTestFailure(const FString& Message, const char* File, int32 Line);

struct FTestRegistrar
{
	FTestRegistrar(const char* Name, void (*Function)())
	{
		GetTestRegistry().push_back({Name, Function});
	}
};

template <typename LeftType, typename RightType>
void CheckEqual(const LeftType& Left, const RightType& Right, const char* LeftText, const char* RightText, const char* File, int32 Line)
{
	if (!(Left == Right))
	{
		std::ostringstream Message;
		Message << LeftText << " == " << RightText << " (got " << Left << " vs " << Right << ")";
		ReportTestFailure(Message.str(), File, Line);
	}
}

#define TEST_CASE(TestName) \
	static void TestName(); \
	static FTestRegistrar Registrar_##TestName(#TestName, &TestName); \
	static void TestName()

#define CHECK(Expression) \
	do { if (!(Expression)) { ReportTestFailure(#Expression, __FILE__, __LINE__); } } while (false)

#define CHECK_EQ(Left, Right) CheckEqual((Left), (Right), #Left, #Right, __FILE__, __LINE__)
