#include "SolidCredits.h"

void SolidCredits::CollectLines(TArray<FString>& OutLines)
{
	OutLines.Reset();
	OutLines.Add(TEXT("Credits"));
	OutLines.Add(TEXT(""));
	OutLines.Add(TEXT("SolidCore1"));
	OutLines.Add(FString::Printf(TEXT("Author  %s"), ProjectAuthor));
	OutLines.Add(FString::Printf(TEXT("Built with %s"), BuiltWith));
	OutLines.Add(TEXT(""));
	OutLines.Add(TEXT("Fab assets"));
	OutLines.Add(FString::Printf(TEXT("Viking  %s"), VikingCreator));
	OutLines.Add(FString::Printf(TEXT("  %s"), VikingLicense));
	OutLines.Add(FString::Printf(TEXT("  %s"), VikingListing));
	OutLines.Add(FString::Printf(TEXT("025 Grass  %s"), GrassCreator));
	OutLines.Add(FString::Printf(TEXT("  %s"), GrassLicense));
	OutLines.Add(FString::Printf(TEXT("  %s"), GrassListing));
	OutLines.Add(TEXT(""));
	OutLines.Add(ToggleHint);
}
