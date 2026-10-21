#include "SolidTerrainSeat.h"
#include "SolidTerrainMap.h"
#include "SolidTerrainStreamer.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"

namespace SolidTerrainSeat
{
	namespace
	{
		FVector2D ToLocal(
			const FVector& ActorLocation,
			const float YawDeg,
			const float WorldX,
			const float WorldY)
		{
			const FVector Rotated = FRotator(0.f, -YawDeg, 0.f).RotateVector(
				FVector(WorldX - ActorLocation.X, WorldY - ActorLocation.Y, 0.f));
			return FVector2D(Rotated.X, Rotated.Y);
		}

		FVector2D ToWorld(
			const FVector& ActorLocation,
			const float YawDeg,
			const FVector2D& Local)
		{
			const FVector Rotated = FRotator(0.f, YawDeg, 0.f).RotateVector(FVector(Local.X, Local.Y, 0.f));
			return FVector2D(ActorLocation.X + Rotated.X, ActorLocation.Y + Rotated.Y);
		}

		float HalfXCm(const FFootprint& Footprint)
		{
			return FMath::Max(Footprint.HalfXCm, 0.f);
		}

		float HalfYCm(const FFootprint& Footprint)
		{
			return Footprint.bRound ? HalfXCm(Footprint) : FMath::Max(Footprint.HalfYCm, 0.f);
		}
	}

	bool Contains(
		const FVector& ActorLocation,
		const float YawDeg,
		const FFootprint& Footprint,
		const float WorldX,
		const float WorldY)
	{
		const FVector2D Local = ToLocal(ActorLocation, YawDeg, WorldX, WorldY);
		const float X = Local.X - Footprint.LocalCenter.X;
		const float Y = Local.Y - Footprint.LocalCenter.Y;
		if (Footprint.bRound)
		{
			const float Radius = HalfXCm(Footprint);
			return X * X + Y * Y <= Radius * Radius;
		}
		return FMath::Abs(X) <= HalfXCm(Footprint) && FMath::Abs(Y) <= HalfYCm(Footprint);
	}

	float LowestCm(
		const USolidTerrainMap* Terrain,
		const FVector& ActorLocation,
		const float YawDeg,
		const FFootprint& Footprint)
	{
		if (!Terrain || !Terrain->IsBuilt())
		{
			return 0.f;
		}

		float Lowest = TNumericLimits<float>::Max();
		int32 Count = 0;
		auto Consider = [&](const float WorldX, const float WorldY)
		{
			Lowest = FMath::Min(Lowest, Terrain->SampleHeight(WorldX, WorldY));
			++Count;
		};
		auto ConsiderLocal = [&](const FVector2D& Local)
		{
			const FVector2D World = ToWorld(ActorLocation, YawDeg, Local);
			Consider(World.X, World.Y);
		};

		const FVector2D Center = Footprint.LocalCenter;
		ConsiderLocal(Center);

		const float ExtentX = HalfXCm(Footprint);
		const float ExtentY = HalfYCm(Footprint);
		float BoundX = ExtentX;
		float BoundY = ExtentY;
		if (Footprint.bRound)
		{
			const int32 RimCount = RoundSampleCount(ExtentX, Terrain->PointSpacing);
			for (int32 Step = 0; Step < RimCount; ++Step)
			{
				const float Angle = 2.f * PI * static_cast<float>(Step) / static_cast<float>(RimCount);
				ConsiderLocal(Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * ExtentX);
			}
		}
		else
		{
			const float Step = SampleStepCm(Terrain->PointSpacing);
			const FVector2D Corners[4] = {
				Center + FVector2D(-ExtentX, -ExtentY),
				Center + FVector2D(ExtentX, -ExtentY),
				Center + FVector2D(ExtentX, ExtentY),
				Center + FVector2D(-ExtentX, ExtentY),
			};
			for (int32 Edge = 0; Edge < 4; ++Edge)
			{
				const FVector2D Start = Corners[Edge];
				const FVector2D End = Corners[(Edge + 1) % 4];
				const float Length = FVector2D::Distance(Start, End);
				const int32 Pieces = FMath::Max(1, FMath::CeilToInt(Length / Step));
				for (int32 Piece = 0; Piece <= Pieces; ++Piece)
				{
					const float Along = static_cast<float>(Piece) / static_cast<float>(Pieces);
					ConsiderLocal(FMath::Lerp(Start, End, Along));
				}
			}
			const float YawRad = FMath::DegreesToRadians(YawDeg);
			const float CosYaw = FMath::Abs(FMath::Cos(YawRad));
			const float SinYaw = FMath::Abs(FMath::Sin(YawRad));
			BoundX = ExtentX * CosYaw + ExtentY * SinYaw;
			BoundY = ExtentX * SinYaw + ExtentY * CosYaw;
		}

		const FVector2D WorldCenter = ToWorld(ActorLocation, YawDeg, Center);
		const float Spacing = FMath::Max(Terrain->PointSpacing, 1.f);
		const int32 MinX = FMath::Clamp(
			FMath::FloorToInt((WorldCenter.X - BoundX - Terrain->OriginXY.X) / Spacing),
			0,
			Terrain->GridWidth - 1);
		const int32 MaxX = FMath::Clamp(
			FMath::CeilToInt((WorldCenter.X + BoundX - Terrain->OriginXY.X) / Spacing),
			0,
			Terrain->GridWidth - 1);
		const int32 MinY = FMath::Clamp(
			FMath::FloorToInt((WorldCenter.Y - BoundY - Terrain->OriginXY.Y) / Spacing),
			0,
			Terrain->GridHeight - 1);
		const int32 MaxY = FMath::Clamp(
			FMath::CeilToInt((WorldCenter.Y + BoundY - Terrain->OriginXY.Y) / Spacing),
			0,
			Terrain->GridHeight - 1);

		for (int32 IndexY = MinY; IndexY <= MaxY; ++IndexY)
		{
			for (int32 IndexX = MinX; IndexX <= MaxX; ++IndexX)
			{
				const FSolidTerrainPoint& Point = Terrain->GetPoint(IndexX, IndexY);
				if (Contains(ActorLocation, YawDeg, Footprint, Point.X, Point.Y))
				{
					Consider(Point.X, Point.Y);
				}
			}
		}

		if (Count == 0)
		{
			return 0.f;
		}
		return Lowest;
	}

	float DropCm(
		const USolidTerrainMap* Terrain,
		const float SurfaceBias,
		const FVector& ActorLocation,
		const float YawDeg,
		const FFootprint& Footprint)
	{
		if (!Terrain || !Terrain->IsBuilt())
		{
			return 0.f;
		}
		const float Target = LowestCm(Terrain, ActorLocation, YawDeg, Footprint) + SurfaceBias - BurialCm;
		return FMath::Max(0.f, ActorLocation.Z - Target);
	}

	float DropCm(const AActor* Actor, const FFootprint& Footprint)
	{
		if (!Actor)
		{
			return 0.f;
		}
		UWorld* World = Actor->GetWorld();
		if (!World)
		{
			return 0.f;
		}

		const ASolidTerrainStreamer* Streamer = nullptr;
		for (TActorIterator<ASolidTerrainStreamer> It(World); It; ++It)
		{
			Streamer = *It;
			break;
		}
		if (!Streamer)
		{
			return 0.f;
		}
		return DropCm(
			Streamer->GetTerrainMap(),
			Streamer->CollisionHeightBias,
			Actor->GetActorLocation(),
			Actor->GetActorRotation().Yaw,
			Footprint);
	}

	void BuryCentered(
		UStaticMeshComponent* Part,
		const float BottomZ,
		const float TopZ,
		const float InDropCm,
		const float ShapeHeightCm)
	{
		if (!Part || InDropCm <= 0.f || ShapeHeightCm <= KINDA_SMALL_NUMBER)
		{
			return;
		}

		const float NewBottom = BottomZ - InDropCm;
		const float NewHeight = TopZ - NewBottom;
		FVector Location = Part->GetRelativeLocation();
		Location.Z = (NewBottom + TopZ) * 0.5f;
		FVector Scale = Part->GetRelativeScale3D();
		Scale.Z = NewHeight / ShapeHeightCm;
		Part->SetRelativeLocation(Location);
		Part->SetRelativeScale3D(Scale);
	}
}
