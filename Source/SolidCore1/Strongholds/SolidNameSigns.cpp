#include "SolidNameSigns.h"
#include "Towns/SolidTownSigns.h"

namespace
{
	struct FLetter
	{
		FKey Key;
		TCHAR Lower = 0;
		TCHAR Upper = 0;
	};

	struct FDigit
	{
		FKey Key;
		TCHAR Char = 0;
	};

	const FLetter Letters[] = {
		{ EKeys::A, TEXT('a'), TEXT('A') },
		{ EKeys::B, TEXT('b'), TEXT('B') },
		{ EKeys::C, TEXT('c'), TEXT('C') },
		{ EKeys::D, TEXT('d'), TEXT('D') },
		{ EKeys::E, TEXT('e'), TEXT('E') },
		{ EKeys::F, TEXT('f'), TEXT('F') },
		{ EKeys::G, TEXT('g'), TEXT('G') },
		{ EKeys::H, TEXT('h'), TEXT('H') },
		{ EKeys::I, TEXT('i'), TEXT('I') },
		{ EKeys::J, TEXT('j'), TEXT('J') },
		{ EKeys::K, TEXT('k'), TEXT('K') },
		{ EKeys::L, TEXT('l'), TEXT('L') },
		{ EKeys::M, TEXT('m'), TEXT('M') },
		{ EKeys::N, TEXT('n'), TEXT('N') },
		{ EKeys::O, TEXT('o'), TEXT('O') },
		{ EKeys::P, TEXT('p'), TEXT('P') },
		{ EKeys::Q, TEXT('q'), TEXT('Q') },
		{ EKeys::R, TEXT('r'), TEXT('R') },
		{ EKeys::S, TEXT('s'), TEXT('S') },
		{ EKeys::T, TEXT('t'), TEXT('T') },
		{ EKeys::U, TEXT('u'), TEXT('U') },
		{ EKeys::V, TEXT('v'), TEXT('V') },
		{ EKeys::W, TEXT('w'), TEXT('W') },
		{ EKeys::X, TEXT('x'), TEXT('X') },
		{ EKeys::Y, TEXT('y'), TEXT('Y') },
		{ EKeys::Z, TEXT('z'), TEXT('Z') },
	};

	const FDigit Digits[] = {
		{ EKeys::Zero, TEXT('0') },
		{ EKeys::One, TEXT('1') },
		{ EKeys::Two, TEXT('2') },
		{ EKeys::Three, TEXT('3') },
		{ EKeys::Four, TEXT('4') },
		{ EKeys::Five, TEXT('5') },
		{ EKeys::Six, TEXT('6') },
		{ EKeys::Seven, TEXT('7') },
		{ EKeys::Eight, TEXT('8') },
		{ EKeys::Nine, TEXT('9') },
		{ EKeys::NumPadZero, TEXT('0') },
		{ EKeys::NumPadOne, TEXT('1') },
		{ EKeys::NumPadTwo, TEXT('2') },
		{ EKeys::NumPadThree, TEXT('3') },
		{ EKeys::NumPadFour, TEXT('4') },
		{ EKeys::NumPadFive, TEXT('5') },
		{ EKeys::NumPadSix, TEXT('6') },
		{ EKeys::NumPadSeven, TEXT('7') },
		{ EKeys::NumPadEight, TEXT('8') },
		{ EKeys::NumPadNine, TEXT('9') },
	};

	TCHAR CharacterFor(const FKey& Key, const bool bUpper)
	{
		if (Key == EKeys::SpaceBar)
		{
			return TEXT(' ');
		}
		for (const FLetter& Letter : Letters)
		{
			if (Letter.Key == Key)
			{
				return bUpper ? Letter.Upper : Letter.Lower;
			}
		}
		for (const FDigit& Digit : Digits)
		{
			if (Digit.Key == Key)
			{
				return Digit.Char;
			}
		}
		return 0;
	}
}

FString SolidNameSigns::Keep(const FString& Typed)
{
	FString Trimmed = Typed.TrimStartAndEnd();
	if (Trimmed.Len() > MaxCharacters)
	{
		Trimmed = Trimmed.Left(MaxCharacters).TrimEnd();
	}
	return Trimmed;
}

bool SolidNameSigns::ApplyKey(FString& Line, const FKey& Key, const bool bUpper)
{
	if (Key == EKeys::BackSpace)
	{
		if (Line.IsEmpty())
		{
			return false;
		}
		Line.LeftChopInline(1);
		return true;
	}

	const TCHAR Char = CharacterFor(Key, bUpper);
	if (Char == 0 || Line.Len() >= MaxCharacters)
	{
		return false;
	}
	Line.AppendChar(Char);
	return true;
}

float SolidNameSigns::BoardWidthCm(const FString& Name)
{
	return SolidTownSigns::BoardWidthCm(Name);
}

float SolidNameSigns::BoardHeightCm()
{
	return SolidTownSigns::TextWorldSize * 1.6f;
}

FVector SolidNameSigns::PoleLocalCm(const FSolidNameSignPlace& Place, const FString& Name)
{
	const FVector2D Out = Place.Outward.GetSafeNormal();
	const FVector2D Side = Out.IsNearlyZero() ? FVector2D(0.f, 1.f) : Out;
	const float Reach = StepCm + BoardWidthCm(Name) * 0.5f;
	return Place.FrameEdgeCm + FVector(Side.X, Side.Y, 0.f) * Reach;
}

SolidNameSigns::FWorldPlace SolidNameSigns::WorldPlace(
	const FSolidStronghold& Stronghold,
	const USolidWorldChart* Map)
{
	const float Yaw = Stronghold.ExteriorYawDeg(Map);
	const FSolidNameSignPlace Place = Stronghold.NameSign();
	const FVector Local = PoleLocalCm(Place, Stronghold.Name);
	const FVector WorldPoint = FTransform(
		FRotator(0.f, Yaw, 0.f),
		FVector(Stronghold.Location.X, Stronghold.Location.Y, 0.f)).TransformPosition(Local);
	FWorldPlace Out;
	Out.XY = FVector2D(WorldPoint.X, WorldPoint.Y);
	Out.YawDeg = Yaw + Place.YawDeg;
	return Out;
}
