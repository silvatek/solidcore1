#include "SolidArrangeSign.h"

#include "Strongholds/Dungeon/SolidDungeonLevel.h"
#include "Strongholds/Furnishings/SolidFloorSign.h"
#include "Strongholds/Furnishings/SolidStrongholdGrid.h"
#include "Strongholds/Tower/SolidTower.h"
#include "SolidMaterials.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

namespace
{
	bool GlyphRows(const TCHAR Letter, uint8 OutRows[7])
	{
		const uint8* Rows = nullptr;
		switch (Letter)
		{
		case TEXT('A'):
		{
			static const uint8 Bits[] = { 14, 17, 17, 31, 17, 17, 17 };
			Rows = Bits;
			break;
		}
		case TEXT('a'):
		{
			static const uint8 Bits[] = { 0, 0, 14, 1, 15, 17, 15 };
			Rows = Bits;
			break;
		}
		case TEXT('e'):
		{
			static const uint8 Bits[] = { 0, 0, 14, 31, 16, 17, 14 };
			Rows = Bits;
			break;
		}
		case TEXT('f'):
		{
			static const uint8 Bits[] = { 6, 8, 28, 8, 8, 8, 8 };
			Rows = Bits;
			break;
		}
		case TEXT('g'):
		{
			static const uint8 Bits[] = { 0, 0, 15, 17, 15, 1, 14 };
			Rows = Bits;
			break;
		}
		case TEXT('i'):
		{
			static const uint8 Bits[] = { 4, 0, 12, 4, 4, 4, 14 };
			Rows = Bits;
			break;
		}
		case TEXT('n'):
		{
			static const uint8 Bits[] = { 0, 0, 28, 18, 17, 17, 17 };
			Rows = Bits;
			break;
		}
		case TEXT('r'):
		{
			static const uint8 Bits[] = { 0, 0, 14, 16, 16, 16, 16 };
			Rows = Bits;
			break;
		}
		case TEXT('t'):
		{
			static const uint8 Bits[] = { 4, 4, 14, 4, 4, 4, 2 };
			Rows = Bits;
			break;
		}
		case TEXT('u'):
		{
			static const uint8 Bits[] = { 0, 0, 17, 17, 17, 17, 15 };
			Rows = Bits;
			break;
		}
		default:
			return false;
		}
		for (int32 Row = 0; Row < 7; ++Row)
		{
			OutRows[Row] = Rows[Row];
		}
		return true;
	}

	float LineWidthCm(const int32 LetterCount)
	{
		const float Pitch = 5.f * 2.4f + 1.4f;
		return static_cast<float>(LetterCount) * Pitch - 1.4f;
	}
}

float FSolidArrangeSign::NaturalPlateWidthCm()
{
	return LineWidthCm(9) + PlatePadCm * 2.f;
}

FVector FSolidArrangeSign::PlateSizeCm() const
{
	const float Width = NaturalPlateWidthCm();
	const float Height = (7.f * CellCm) * 2.f + LineGapCm + PlatePadCm * 2.f;
	return FVector(PlateDepthCm, Width, Height) * MetricScale;
}

FLinearColor FSolidArrangeSign::PlateColor()
{
	return SolidStronghold::FSolidFloorSign::PlateColor();
}

FLinearColor FSolidArrangeSign::TextColor()
{
	const FColor Ink = SolidStronghold::FSolidFloorSign::TextColor();
	return FLinearColor(Ink.R / 255.f, Ink.G / 255.f, Ink.B / 255.f, 1.f);
}

void FSolidArrangeSign::Place(const FVector& InLocation, const float InYawDeg)
{
	StandLocation = InLocation;
	FacingYawDeg = InYawDeg;
	bPresent = true;
}

void FSolidArrangeSign::CollectStrokes(TArray<FStroke>& Out) const
{
	Out.Reset();
	if (!bPresent)
	{
		return;
	}

	const TCHAR* Lines[] = { TEXT("Arrange"), TEXT("furniture") };
	const float Pitch = 5.f * CellCm + LetterGapCm;
	const float LineHeight = 7.f * CellCm;
	const float BlockHeight = LineHeight * 2.f + LineGapCm;
	const float TopZ = BlockHeight * 0.5f - LineHeight * 0.5f;

	for (int32 LineIndex = 0; LineIndex < 2; ++LineIndex)
	{
		const TCHAR* Line = Lines[LineIndex];
		const int32 Count = FCString::Strlen(Line);
		const float Width = LineWidthCm(Count);
		float CursorY = Width * 0.5f;
		const float LineZ = TopZ - static_cast<float>(LineIndex) * (LineHeight + LineGapCm);
		for (int32 LetterIndex = 0; LetterIndex < Count; ++LetterIndex)
		{
			uint8 Rows[7] = {};
			if (GlyphRows(Line[LetterIndex], Rows))
			{
				const float LetterCenterY = CursorY - (5.f * CellCm) * 0.5f;
				for (int32 Row = 0; Row < 7; ++Row)
				{
					for (int32 Col = 0; Col < 5; ++Col)
					{
						if ((Rows[Row] & (1 << (4 - Col))) == 0)
						{
							continue;
						}
						FStroke Stroke;
						const float LocalY = LetterCenterY + (2.f - static_cast<float>(Col)) * CellCm;
						const float LocalZ = LineZ + (3.f - static_cast<float>(Row)) * CellCm;
						Stroke.CenterCm = FVector(StrokeDepthCm * 0.5f, LocalY, LocalZ) * MetricScale;
						Stroke.SizeCm = FVector(StrokeDepthCm, CellCm, CellCm) * MetricScale;
						Out.Add(Stroke);
					}
				}
			}
			CursorY -= Pitch;
		}
	}
}

FVector FSolidArrangeSign::PlateLocation() const
{
	const FVector Inward = FRotator(0.f, FacingYawDeg, 0.f).Vector();
	const float TowardWall = (StrokeDepthCm + PlateGapCm + PlateDepthCm * 0.5f) * MetricScale;
	return StandLocation - Inward * TowardWall;
}

FSolidArrangeSign FSolidArrangeSign::ForCave()
{
	const SolidStrongholdGrid::FCell EastCell{ 2, 0 };
	const FVector2D Center = SolidStrongholdGrid::HexCenter(
		EastCell.Col,
		EastCell.Row,
		SolidDungeonLevel::MeagreOrigin());
	const float WallX = Center.X + SolidStrongholdGrid::FlatToFlatCm * 0.5f;
	FSolidArrangeSign Sign;
	// Each wall cube runs past its vertex by half of this overlap, and that rock covers the face.
	const float CornerCm = SolidDungeonLevel::ShellCm * 0.35f * 0.5f;
	const float FaceCm = SolidStrongholdGrid::SideCm() - CornerCm * 2.f;
	Sign.MetricScale = FaceCm / NaturalPlateWidthCm();
	const float Depth = PlateDepthCm * Sign.MetricScale;
	const float TowardWall = (StrokeDepthCm + PlateGapCm + PlateDepthCm * 0.5f) * Sign.MetricScale;
	Sign.Place(
		FVector(WallX - Depth * 0.5f - TowardWall, Center.Y, 150.f),
		180.f);
	return Sign;
}

FSolidArrangeSign FSolidArrangeSign::ForTowerFloor(const SolidStronghold::FSolidTower& Tower, const int32 Floor)
{
	FSolidArrangeSign Sign;
	if (Floor < 0 || Floor >= Tower.FloorCount())
	{
		return Sign;
	}

	const FVector2D Door = Tower.DoorOutward();
	float Angle = FMath::Atan2(Door.Y, Door.X) + PI;
	const SolidStronghold::FSolidFloorSign Number = SolidStronghold::FloorSignFor(Tower.Plan(), Floor);
	if (Number.IsPresent())
	{
		const float NumberAngle = FMath::Atan2(Number.Location().Y, Number.Location().X);
		const float Delta = FMath::Abs(FMath::FindDeltaAngleRadians(Angle, NumberAngle));
		if (Delta < 0.9f)
		{
			Angle += PI * 0.5f;
		}
	}

	const float SignZ = Tower.FloorZCm(Floor) + SolidStronghold::FloorSignLiftCm;
	const float Radius = FMath::Max(Tower.InteriorRadiusCm(SignZ) - SolidStronghold::FloorSignInsetCm, 40.f);
	const float Yaw = FMath::RadiansToDegrees(Angle + PI);
	Sign.Place(
		FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, SignZ),
		Yaw);
	return Sign;
}

void FSolidArrangeSign::ApplyPlate(
	UStaticMeshComponent* Plate,
	UStaticMesh* Cube,
	const float CubeSizeCm) const
{
	if (!Plate || !Cube || !bPresent || CubeSizeCm <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const FRotator Facing(0.f, FacingYawDeg, 0.f);
	Plate->SetStaticMesh(Cube);
	Plate->SetRelativeLocation(PlateLocation());
	Plate->SetRelativeRotation(Facing);
	Plate->SetRelativeScale3D(PlateSizeCm() / CubeSizeCm);
	Plate->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Plate->SetCollisionResponseToAllChannels(ECR_Ignore);
	Plate->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Plate->SetCastShadow(false);
	Plate->SetVisibility(true);
	if (UMaterialInterface* Material = SolidMaterials::CreateSolidColor(
		Plate->GetOwner(),
		PlateColor(),
		TEXT("ArrangePlate"),
		0.88f))
	{
		Plate->SetMaterial(0, Material);
	}
}

void FSolidArrangeSign::AppendLetters(UInstancedStaticMeshComponent* Instances, const float CubeSizeCm) const
{
	if (!Instances || !bPresent || CubeSizeCm <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const FRotator Facing(0.f, FacingYawDeg, 0.f);
	TArray<FStroke> Strokes;
	CollectStrokes(Strokes);
	for (const FStroke& Stroke : Strokes)
	{
		Instances->AddInstance(FTransform(
			Facing,
			StandLocation + Facing.RotateVector(Stroke.CenterCm),
			Stroke.SizeCm / CubeSizeCm));
	}
}
