#pragma once

#include "CoreMinimal.h"

/**
 * Journal clock. A new play session advances the date by one day.
 * Hours and minutes match real time since that session started, and do not roll the date.
 * Days are 0–99. The first session is 200/45 AtF.
 */
namespace SolidWorldTime
{
	inline constexpr int32 StartYear = 200;
	inline constexpr int32 StartDay = 45;
	inline constexpr int32 DaysPerYear = 100;
	inline constexpr const TCHAR* Era = TEXT("AtF");

	struct FDate
	{
		int32 Year = StartYear;
		int32 Day = StartDay;
	};

	/** MinuteOfSession is minutes since the session started, not minutes past midnight. */
	struct FStamp
	{
		int32 Year = StartYear;
		int32 Day = StartDay;
		int32 MinuteOfSession = 0;
	};

	/** Completed sessions already played. The next launch is one higher, and the first is 1. */
	inline int32 NextSessionNumber(const int32 CompletedSessions)
	{
		return FMath::Max(0, CompletedSessions) + 1;
	}

	/** Session 1 is 200/45. Session 9 is 200/53. Session 56 is 201/0. */
	inline FDate DateForSession(const int32 SessionNumber)
	{
		const int32 ElapsedDays = FMath::Max(1, SessionNumber) - 1;
		const int32 AbsoluteDay = StartDay + ElapsedDays;
		FDate Date;
		Date.Year = StartYear + AbsoluteDay / DaysPerYear;
		Date.Day = AbsoluteDay % DaysPerYear;
		return Date;
	}

	/** Truncates to the minute. 84 minutes is 01:24. A long sitting can read 26:10 on the same date. */
	inline FStamp FromSession(const int32 SessionNumber, const double ElapsedSeconds)
	{
		const FDate Date = DateForSession(SessionNumber);
		FStamp Stamp;
		Stamp.Year = Date.Year;
		Stamp.Day = Date.Day;
		const double Clamped = FMath::Clamp(ElapsedSeconds, 0.0, 60.0 * 60.0 * 10000.0);
		Stamp.MinuteOfSession = FMath::FloorToInt(Clamped / 60.0);
		return Stamp;
	}

	/** "01:24, 200/53 AtF". */
	inline FString Format(const FStamp& Stamp)
	{
		const int32 Minutes = FMath::Max(0, Stamp.MinuteOfSession);
		const int32 Hour = Minutes / 60;
		const int32 Minute = Minutes % 60;
		return FString::Printf(TEXT("%02d:%02d, %d/%d %s"), Hour, Minute, Stamp.Year, Stamp.Day, Era);
	}
}
