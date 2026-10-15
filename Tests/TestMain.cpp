#include "TestFramework.h"

namespace
{
	int32 FailedChecks = 0;
}

TArray<FTestCase>& GetTestRegistry()
{
	static TArray<FTestCase> Registry;
	return Registry;
}

void ReportTestFailure(const FString& Message, const char* File, int32 Line)
{
	++FailedChecks;
	std::cout << "    FAILED: " << Message << "  [" << File << ":" << Line << "]\n";
}

int main()
{
	int32 FailedTests = 0;
	for (const FTestCase& Test : GetTestRegistry())
	{
		const int32 FailuresBefore = FailedChecks;
		Test.Function();
		const bool bPassed = (FailedChecks == FailuresBefore);
		std::cout << (bPassed ? "[ PASS ] " : "[ FAIL ] ") << Test.Name << "\n";
		if (!bPassed)
		{
			++FailedTests;
		}
	}

	std::cout << "\n" << (GetTestRegistry().size() - static_cast<size_t>(FailedTests)) << "/" << GetTestRegistry().size()
			  << " tests passed\n";
	return FailedTests == 0 ? 0 : 1;
}
