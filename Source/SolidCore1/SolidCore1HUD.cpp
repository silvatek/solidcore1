#include "SolidCore1HUD.h"
#include "SolidCore1BuildId.h"
#include "SolidCore1Character.h"
#include "Companion/SolidCore1CompanionCharacter.h"
#include "Terrain/SolidCore1TerrainMap.h"
#include "Terrain/SolidCore1TerrainStreamer.h"
#include "Terrain/SolidCore1TerrainTypes.h"
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

namespace SolidCore1HUDPrivate
{
	static ASolidCore1TerrainStreamer* FindStreamer(UWorld* World)
	{
		if (!World)
		{
			return nullptr;
		}
		for (TActorIterator<ASolidCore1TerrainStreamer> It(World); It; ++It)
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

void ASolidCore1HUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas)
	{
		return;
	}

	TArray<FString> Lines;
	Lines.Add(FString::Printf(TEXT("Build %s"), SOLIDCORE1_BUILD_ID));
	Lines.Add(FString::Printf(TEXT("Change: %s"), SOLIDCORE1_BUILD_NOTE));

	APawn* Pawn = GetOwningPawn();
	ASolidCore1TerrainStreamer* Streamer = SolidCore1HUDPrivate::FindStreamer(GetWorld());

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

			const FSolidCore1TerrainPoint Point = Streamer->GetTerrainPointAt(Loc);
			Lines.Add(FString::Printf(
				TEXT("Biome %s  threat=%.2f  fog=%.2f"),
				SolidCore1TerrainTypes::BiomeToString(Point.Biome),
				Point.Threat, Point.Fog));
			if (const USolidCore1TerrainMap* Map = Streamer->GetTerrainMap())
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
			if (const ASolidCore1Character* SolidCharacter = Cast<ASolidCore1Character>(Character))
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
			if (const ASolidCore1Character* SolidCharacter = Cast<ASolidCore1Character>(Pawn))
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
		ASolidCore1CompanionCharacter* Companion = nullptr;
		if (UWorld* World = GetWorld())
		{
			for (TActorIterator<ASolidCore1CompanionCharacter> It(World); It; ++It)
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
	SolidCore1HUDPrivate::DrawLines(Canvas, Font, Lines, FLinearColor::White);
}
