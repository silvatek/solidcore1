#include "SolidHUD.h"
#include "SolidBuildId.h"
#include "SolidCharacter.h"
#include "Companion/SolidCompanionCharacter.h"
#include "Terrain/SolidTerrainMap.h"
#include "Terrain/SolidTerrainStreamer.h"
#include "Terrain/SolidTerrainTypes.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/SkeletalMesh.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInterface.h"

namespace SolidHUDPrivate
{
	static ASolidTerrainStreamer* FindStreamer(UWorld* World)
	{
		if (!World)
		{
			return nullptr;
		}
		for (TActorIterator<ASolidTerrainStreamer> It(World); It; ++It)
		{
			return *It;
		}
		return nullptr;
	}

	static void DrawLines(
		UCanvas* Canvas,
		UFont* Font,
		const TArray<FString>& Lines,
		const FLinearColor& TextColor)
	{
		if (!Canvas || Lines.Num() == 0)
		{
			return;
		}

		const float PadX = 16.f;
		const float PadY = 12.f;
		const float LineGap = 2.f;
		const float BoxPad = 6.f;

		float MaxWidth = 0.f;
		float LineHeight = 14.f;
		for (const FString& Line : Lines)
		{
			float W = 0.f;
			float H = 0.f;
			if (Font)
			{
				Canvas->StrLen(Font, Line, W, H);
			}
			else
			{
				W = static_cast<float>(Line.Len() * 8);
				H = 14.f;
			}
			MaxWidth = FMath::Max(MaxWidth, W);
			LineHeight = FMath::Max(LineHeight, H);
		}

		const float BlockHeight = Lines.Num() * LineHeight + (Lines.Num() - 1) * LineGap;
		FCanvasTileItem Background(
			FVector2D(PadX - BoxPad, PadY - BoxPad * 0.5f),
			FVector2D(MaxWidth + BoxPad * 2.f, BlockHeight + BoxPad),
			FLinearColor(0.f, 0.f, 0.f, 0.55f));
		Background.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(Background);

		float Y = PadY;
		for (const FString& Line : Lines)
		{
			FCanvasTextItem TextItem(FVector2D(PadX, Y), FText::FromString(Line), Font, TextColor);
			TextItem.EnableShadow(FLinearColor(0.f, 0.f, 0.f, 0.9f));
			Canvas->DrawItem(TextItem);
			Y += LineHeight + LineGap;
		}
	}
}

void ASolidHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas)
	{
		return;
	}

	TArray<FString> Lines;
	Lines.Add(FString::Printf(TEXT("Build %s"), SOLID_BUILD_ID));
	Lines.Add(FString::Printf(TEXT("Change: %s"), SOLID_BUILD_NOTE));

	const float DeltaSeconds = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.f;
	const double NowSeconds = FPlatformTime::Seconds();
	RecentFrameTimes.Add(NowSeconds);
	const double WindowStart = NowSeconds - static_cast<double>(FMath::Max(FpsAverageWindowSeconds, 0.1f));
	while (RecentFrameTimes.Num() > 0 && RecentFrameTimes[0] < WindowStart)
	{
		RecentFrameTimes.RemoveAt(0, 1, EAllowShrinking::No);
	}
	const double Elapsed = (RecentFrameTimes.Num() >= 2)
		? (RecentFrameTimes.Last() - RecentFrameTimes[0])
		: static_cast<double>(DeltaSeconds);
	const float AvgFps = (Elapsed > KINDA_SMALL_NUMBER)
		? static_cast<float>(FMath::Max(RecentFrameTimes.Num() - 1, 1)) / static_cast<float>(Elapsed)
		: 0.f;
	const float AvgMs = (AvgFps > KINDA_SMALL_NUMBER) ? (1000.f / AvgFps) : (DeltaSeconds * 1000.f);
	Lines.Add(FString::Printf(TEXT("FPS %.0f  (%.1f ms, %.1fs avg)"), AvgFps, AvgMs, FpsAverageWindowSeconds));

	APawn* Pawn = GetOwningPawn();
	ASolidTerrainStreamer* Streamer = SolidHUDPrivate::FindStreamer(GetWorld());

	if (Pawn)
	{
		const FVector Loc = Pawn->GetActorLocation();
		float CapsuleHalf = 96.f;
		if (const ACharacter* Character = Cast<ACharacter>(Pawn))
		{
			if (const UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
			{
				CapsuleHalf = Capsule->GetScaledCapsuleHalfHeight();
			}
		}
		const float FeetZ = Loc.Z - CapsuleHalf;
		Lines.Add(FString::Printf(TEXT("Pawn  %.0f, %.0f, %.0f  (feet Z=%.0f)"), Loc.X, Loc.Y, Loc.Z, FeetZ));

		if (Streamer)
		{
			const float LandZ = Streamer->GetHeightAt(Loc) + Streamer->CollisionHeightBias;
			const float Delta = FeetZ - LandZ;
			const FIntPoint Chunk = Streamer->GetChunkCoordAt(Loc);
			Lines.Add(FString::Printf(TEXT("Terrain Z=%.1f  feetΔ=%+.0f cm"), LandZ, Delta));
			Lines.Add(FString::Printf(
				TEXT("Chunk (%d,%d)  loaded=%d  radius=%d  amp=%.0f  freq=%.5f"),
				Chunk.X, Chunk.Y, Streamer->GetLoadedChunkCount(),
				Streamer->ViewRadiusChunks,
				Streamer->Amplitude, Streamer->FrequencyScale));

			const FSolidTerrainPoint Point = Streamer->GetTerrainPointAt(Loc);
			Lines.Add(FString::Printf(
				TEXT("Biome %s  threat=%.2f  fog=%.2f  mist=%.2f"),
				SolidTerrainTypes::BiomeToString(Point.Biome),
				Point.Threat, Point.Fog, Streamer->GetRenderedFogAmount()));
			if (const USolidTerrainMap* Map = Streamer->GetTerrainMap())
			{
				Lines.Add(FString::Printf(
					TEXT("Map %dx%d  spacing=%.0f  points=%d"),
					Map->GridWidth, Map->GridHeight, Map->PointSpacing, Map->GetPointCount()));
			}

			if (const UMaterialInterface* Mat = Streamer->GetActiveMaterial())
			{
				Lines.Add(FString::Printf(TEXT("Material %s"), *Mat->GetName()));
			}
			else
			{
				Lines.Add(TEXT("Material <null>"));
			}
		}
		else
		{
			Lines.Add(TEXT("Streamer <missing>"));
		}
	}
	else
	{
		Lines.Add(TEXT("Pawn <none>"));
	}

	if (APlayerCameraManager* CamMgr = PlayerOwner ? PlayerOwner->PlayerCameraManager : nullptr)
	{
		const FRotator CamRot = CamMgr->GetCameraRotation();
		const FVector CamLoc = CamMgr->GetCameraLocation();
		float ArmLen = -1.f;
		if (const ACharacter* Character = Cast<ACharacter>(Pawn))
		{
			if (const ASolidCharacter* SolidCharacter = Cast<ASolidCharacter>(Character))
			{
				if (const USpringArmComponent* Boom = SolidCharacter->GetCameraBoom())
				{
					ArmLen = Boom->TargetArmLength;
				}
			}
		}
		if (ArmLen >= 0.f)
		{
			float ZoomLen = ArmLen;
			if (const ASolidCharacter* SolidCharacter = Cast<ASolidCharacter>(Pawn))
			{
				ZoomLen = SolidCharacter->GetUserZoomArmLength();
			}
			Lines.Add(FString::Printf(
				TEXT("Cam pitch=%+.1f  Z=%.0f  arm=%.0f  zoom=%.0f"),
				CamRot.Pitch, CamLoc.Z, ArmLen, ZoomLen));
		}
		else
		{
			Lines.Add(FString::Printf(
				TEXT("Cam pitch=%+.1f  Z=%.0f"), CamRot.Pitch, CamLoc.Z));
		}
	}

	{
		ASolidCompanionCharacter* Companion = nullptr;
		if (UWorld* World = GetWorld())
		{
			for (TActorIterator<ASolidCompanionCharacter> It(World); It; ++It)
			{
				Companion = *It;
				break;
			}
		}

		if (Companion && Pawn)
		{
			const float Dist = FVector::Dist(Pawn->GetActorLocation(), Companion->GetActorLocation());
			const FString MeshName = Companion->GetMesh() && Companion->GetMesh()->GetSkeletalMeshAsset()
				? Companion->GetMesh()->GetSkeletalMeshAsset()->GetName()
				: TEXT("<no mesh>");
			Lines.Add(FString::Printf(TEXT("Companion %s  dist=%.0f cm"), *MeshName, Dist));
		}
		else
		{
			Lines.Add(TEXT("Companion <none>"));
		}
	}

	UFont* Font = GEngine ? GEngine->GetSmallFont() : nullptr;
	SolidHUDPrivate::DrawLines(Canvas, Font, Lines, FLinearColor::White);
}
