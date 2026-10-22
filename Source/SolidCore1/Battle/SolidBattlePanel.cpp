#include "SolidBattlePanel.h"
#include "SolidBattle.h"

namespace
{
	/** Battle time for the Time column. A whole number drops the fraction. */
	FString FormatTime(const float Time)
	{
		const float Rounded = FMath::RoundToFloat(Time);
		if (FMath::IsNearlyEqual(Time, Rounded, 0.05f))
		{
			return FString::Printf(TEXT("%.0f"), Rounded);
		}
		return FString::Printf(TEXT("%.1f"), Time);
	}
}

void FSolidBattlePanel::CollectRows(const FSolidBattle& Battle, TArray<FRow>& OutRows)
{
	OutRows.Reset();
	OutRows.Add({ EKind::Title, FString(), FString(Title) });
	OutRows.Add({ EKind::Heading, FString(TimeHeading), FString(SummaryHeading) });

	if (Battle.Log.Num() <= 0)
	{
		OutRows.Add({ EKind::Empty, FString(), FString(EmptyText) });
		return;
	}

	for (const FSolidBattleLogEntry& Entry : Battle.Log)
	{
		OutRows.Add({ EKind::Entry, FormatTime(Entry.Time), Entry.Summary() });
	}
}
