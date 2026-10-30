#pragma once

#include "CoreMinimal.h"

/**
 * Journal clock. A new play session advances the date by one day.
 * Hours and minutes match real time since that session started, and do not roll the date.
 * Days are 0–99. The first session is 200/45 AtF.
 * The game instance keeps one of these for the play session.
 */
class FSolidWorldTime
{
public:
	static constexpr int32 StartYear = 200;
	static constexpr int32 StartDay = 45;
	static constexpr int32 DaysPerYear = 100;
	static constexpr const TCHAR* Era = TEXT("AtF");

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
	int32 NextSessionNumber(int32 CompletedSessions) const;

	/** Session 1 is 200/45. Session 9 is 200/53. Session 56 is 201/0. */
	FDate DateForSession(int32 InSessionNumber) const;

	/** Truncates to the minute. 84 minutes is 01:24. A long sitting can read 26:10 on the same date. */
	FStamp FromSession(int32 InSessionNumber, double ElapsedSeconds) const;

	/** "01:24, 200/53 AtF". */
	FString Format(const FStamp& Stamp) const;

	/** Records the next session and the platform time it started. */
	void Begin(int32 CompletedSessions, double NowSeconds);

	/**
	 * The clock as of NowSeconds. Before Begin, that is session 1 at minute 0.
	 * Hours do not roll the date.
	 */
	FStamp Stamp(double NowSeconds) const;

	int32 GetSessionNumber() const;

private:
	/** 1 on the first launch. Not reset by the cave trip. */
	int32 SessionNumber = 1;
	/** Platform seconds when this play session started. 0 until Begin. */
	double SessionStartSeconds = 0.0;
};
