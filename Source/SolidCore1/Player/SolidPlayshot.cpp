#include "SolidPlayshot.h"

namespace
{
	bool NameTaken(const TArray<FString>& ExistingNames, const FString& Candidate)
	{
		for (const FString& Name : ExistingNames)
		{
			if (Name.Equals(Candidate, ESearchCase::CaseSensitive))
			{
				return true;
			}
		}
		return false;
	}
}

FString SolidPlayshot::FileName(
	const FString& BuildId,
	const FDateTime& Time,
	const TArray<FString>& ExistingNames,
	TArray<FString>& NamesForBuild)
{
	const FString Prefix = BuildId + TEXT("_");
	NamesForBuild.Reset();
	for (const FString& Name : ExistingNames)
	{
		if (Name.StartsWith(Prefix))
		{
			NamesForBuild.Add(Name);
		}
	}

	const FString Stamp = FString::Printf(
		TEXT("%s%04d%02d%02d-%02d%02d%02d"),
		*Prefix,
		Time.GetYear(),
		Time.GetMonth(),
		Time.GetDay(),
		Time.GetHour(),
		Time.GetMinute(),
		Time.GetSecond());

	FString Candidate = Stamp + TEXT(".png");
	if (NameTaken(ExistingNames, Candidate))
	{
		for (int32 Suffix = 2; ; ++Suffix)
		{
			Candidate = FString::Printf(TEXT("%s_%d.png"), *Stamp, Suffix);
			if (!NameTaken(ExistingNames, Candidate))
			{
				break;
			}
		}
	}
	return Candidate;
}
