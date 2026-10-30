#include "SolidWorldTime.h"

int32 FSolidWorldTime::NextSessionNumber(const int32 CompletedSessions) const
{
	return FMath::Max(0, CompletedSessions) + 1;
}

FSolidWorldTime::FDate FSolidWorldTime::DateForSession(const int32 InSessionNumber) const
{
	const int32 ElapsedDays = FMath::Max(1, InSessionNumber) - 1;
	const int32 AbsoluteDay = StartDay + ElapsedDays;
	FDate Date;
	Date.Year = StartYear + AbsoluteDay / DaysPerYear;
	Date.Day = AbsoluteDay % DaysPerYear;
	return Date;
}

FSolidWorldTime::FStamp FSolidWorldTime::FromSession(const int32 InSessionNumber, const double ElapsedSeconds) const
{
	const FDate Date = DateForSession(InSessionNumber);
	FStamp Stamp;
	Stamp.Year = Date.Year;
	Stamp.Day = Date.Day;
	const double Clamped = FMath::Clamp(ElapsedSeconds, 0.0, 60.0 * 60.0 * 10000.0);
	Stamp.MinuteOfSession = FMath::FloorToInt(Clamped / 60.0);
	return Stamp;
}

FString FSolidWorldTime::Format(const FStamp& Stamp) const
{
	const int32 Minutes = FMath::Max(0, Stamp.MinuteOfSession);
	const int32 Hour = Minutes / 60;
	const int32 Minute = Minutes % 60;
	return FString::Printf(TEXT("%02d:%02d, %d/%d %s"), Hour, Minute, Stamp.Year, Stamp.Day, Era);
}

void FSolidWorldTime::Begin(const int32 CompletedSessions, const double NowSeconds)
{
	SessionNumber = NextSessionNumber(CompletedSessions);
	SessionStartSeconds = NowSeconds;
}

FSolidWorldTime::FStamp FSolidWorldTime::Stamp(const double NowSeconds) const
{
	if (SessionStartSeconds <= 0.0)
	{
		return FromSession(SessionNumber, 0.0);
	}
	const double Elapsed = NowSeconds - SessionStartSeconds;
	return FromSession(SessionNumber, Elapsed);
}

int32 FSolidWorldTime::GetSessionNumber() const
{
	return SessionNumber;
}
