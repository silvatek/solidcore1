#pragma once

#include "CoreMinimal.h"

class FSolidBattle;

/**
 * The battle page. It lists the battle log the way the journal lists its lines.
 * The board stays up while that fight is the current one.
 */
class FSolidBattlePanel
{
public:
	static constexpr const TCHAR* Title = TEXT("Battle");
	static constexpr const TCHAR* TimeHeading = TEXT("Time");
	static constexpr const TCHAR* SummaryHeading = TEXT("Summary");
	static constexpr const TCHAR* EmptyText = TEXT("No blows yet.");

	enum class EKind : uint8
	{
		Title,
		Heading,
		Entry,
		Empty,
	};

	struct FRow
	{
		EKind Kind = EKind::Entry;
		/** Battle time. Empty on the title, the heading's words, and the empty line. */
		FString Stamp;
		FString Text;
	};

	/** Title, Time and Summary, then one row for each log line. */
	static void CollectRows(const FSolidBattle& Battle, TArray<FRow>& OutRows);
};
