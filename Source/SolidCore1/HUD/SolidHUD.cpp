#include "SolidHUD.h"
#include "SolidBuildId.h"
#include "Menus/SolidCredits.h"
#include "Menus/SolidJournal.h"
#include "Menus/SolidMainMenu.h"
#include "SolidCharacter.h"
#include "SolidSight.h"
#include "SolidGameMode.h"
#include "Companion/SolidCompanionCharacter.h"
#include "Party/SolidBattlePlan.h"
#include "Party/SolidParty.h"
#include "Party/SolidPartyDrill.h"
#include "Terrain/SolidTerrainMap.h"
#include "Terrain/SolidTerrainStreamer.h"
#include "Terrain/SolidTerrainTypes.h"
#include "GameFramework/GameModeBase.h"
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
	static void DrawCenteredPopup(UCanvas* Canvas, const TArray<FString>& Lines, const float LineGap)
	{
		if (!Canvas || Lines.Num() == 0)
		{
			return;
		}

		UFont* Font = GEngine ? GEngine->GetMediumFont() : nullptr;
		if (!Font && GEngine)
		{
			Font = GEngine->GetSmallFont();
		}

		float MaxWidth = 0.f;
		float LineHeight = 18.f;
		for (const FString& Line : Lines)
		{
			float W = 0.f;
			float H = 0.f;
			if (Font)
			{
				Canvas->StrLen(Font, Line.IsEmpty() ? TEXT(" ") : Line, W, H);
			}
			else
			{
				W = static_cast<float>(FMath::Max(Line.Len(), 1) * 8);
				H = 18.f;
			}
			MaxWidth = FMath::Max(MaxWidth, W);
			LineHeight = FMath::Max(LineHeight, H);
		}

		const float BoxPad = 22.f;
		const float BlockHeight = Lines.Num() * LineHeight + (Lines.Num() - 1) * LineGap;
		const float BoxW = MaxWidth + BoxPad * 2.f;
		const float BoxH = BlockHeight + BoxPad * 2.f;
		const float BoxX = (Canvas->SizeX - BoxW) * 0.5f;
		const float BoxY = (Canvas->SizeY - BoxH) * 0.5f;

		FCanvasTileItem Dim(
			FVector2D(0.f, 0.f),
			FVector2D(static_cast<float>(Canvas->SizeX), static_cast<float>(Canvas->SizeY)),
			FLinearColor(0.f, 0.f, 0.f, 0.72f));
		Dim.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(Dim);

		FCanvasTileItem Panel(
			FVector2D(BoxX, BoxY),
			FVector2D(BoxW, BoxH),
			FLinearColor(0.05f, 0.055f, 0.07f, 1.f));
		Panel.BlendMode = SE_BLEND_Opaque;
		Canvas->DrawItem(Panel);

		float Y = BoxY + BoxPad;
		for (int32 Index = 0; Index < Lines.Num(); ++Index)
		{
			const FString& Line = Lines[Index];
			FLinearColor Color = FLinearColor::White;
			if (Index == 0)
			{
				Color = FLinearColor(1.f, 0.84f, 0.47f);
			}
			else if (Line.StartsWith(TEXT("Fab assets")) || Line.StartsWith(TEXT("SolidCore1")))
			{
				Color = FLinearColor(0.75f, 0.80f, 0.86f);
			}
			else if (Line.StartsWith(TEXT("F10")))
			{
				Color = FLinearColor(0.55f, 0.58f, 0.62f);
			}

			if (!Line.IsEmpty())
			{
				FCanvasTextItem TextItem(FVector2D(BoxX + BoxPad, Y), FText::FromString(Line), Font, Color);
				TextItem.EnableShadow(FLinearColor(0.f, 0.f, 0.f, 0.85f));
				Canvas->DrawItem(TextItem);
			}
			Y += LineHeight + LineGap;
		}
	}

	/** Draw a top-left text block. Returns Y just below the block (including padding). */
	static float DrawLines(
		UCanvas* Canvas,
		UFont* Font,
		const TArray<FString>& Lines,
		const FLinearColor& TextColor,
		float StartY = 12.f)
	{
		if (!Canvas || Lines.Num() == 0)
		{
			return StartY;
		}

		const float PadX = 16.f;
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
			FVector2D(PadX - BoxPad, StartY - BoxPad * 0.5f),
			FVector2D(MaxWidth + BoxPad * 2.f, BlockHeight + BoxPad),
			FLinearColor(0.f, 0.f, 0.f, 0.55f));
		Background.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(Background);

		float Y = StartY;
		for (const FString& Line : Lines)
		{
			FCanvasTextItem TextItem(FVector2D(PadX, Y), FText::FromString(Line), Font, TextColor);
			TextItem.EnableShadow(FLinearColor(0.f, 0.f, 0.f, 0.9f));
			Canvas->DrawItem(TextItem);
			Y += LineHeight + LineGap;
		}

		return Y + BoxPad;
	}

	static FString FormatBattlePlanSlotLine(const int32 SlotIndex, const FSolidBattlePlan* Plan)
	{
		const int32 KeyNumber = SlotIndex + 1;
		if (Plan)
		{
			return FString::Printf(TEXT("F%d  %s"), KeyNumber, *Plan->Name);
		}
		return FString::Printf(TEXT("F%d  —"), KeyNumber);
	}

	/** Battle-plan slot list under the tech HUD. Returns Y below the panel. */
	static float DrawBattlePlansPanel(
		UCanvas* Canvas,
		UFont* Font,
		const USolidParty* Party,
		const float StartY)
	{
		if (!Canvas)
		{
			return StartY;
		}

		const float PadX = 16.f;
		const float LineGap = 2.f;
		const float BoxPad = 6.f;
		const float GapAbove = 10.f;
		const float PanelTop = StartY + GapAbove;

		const int32 ActiveSlot = Party ? Party->GetActiveAssignedSlot() : INDEX_NONE;
		const int32 SlotCount = SolidBattleFormationSlots::MaxAssignedBattlePlans;

		TArray<FString> SlotLines;
		SlotLines.Reserve(SlotCount + 1);
		SlotLines.Add(TEXT("Battle Plans"));
		for (int32 Slot = 0; Slot < SlotCount; ++Slot)
		{
			const FSolidBattlePlan* Plan = Party ? Party->GetAssignedBattlePlan(Slot) : nullptr;
			SlotLines.Add(FormatBattlePlanSlotLine(Slot, Plan));
		}

		// Proportional fonts: don't prefix strings. Draw marker and body at fixed X.
		float MarkerWidth = 10.f;
		float LineHeight = 14.f;
		float BodyMaxWidth = 0.f;
		if (Font)
		{
			float MarkerH = 0.f;
			Canvas->StrLen(Font, TEXT(">"), MarkerWidth, MarkerH);
			LineHeight = FMath::Max(LineHeight, MarkerH);
		}
		const float MarkerGap = 6.f;
		const float BodyX = PadX + MarkerWidth + MarkerGap;

		for (const FString& Line : SlotLines)
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
			BodyMaxWidth = FMath::Max(BodyMaxWidth, W);
			LineHeight = FMath::Max(LineHeight, H);
		}

		const float ContentWidth = (BodyX - PadX) + BodyMaxWidth;
		const float BlockHeight = SlotLines.Num() * LineHeight + (SlotLines.Num() - 1) * LineGap;
		FCanvasTileItem Background(
			FVector2D(PadX - BoxPad, PanelTop - BoxPad * 0.5f),
			FVector2D(ContentWidth + BoxPad * 2.f, BlockHeight + BoxPad),
			FLinearColor(0.f, 0.f, 0.f, 0.55f));
		Background.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(Background);

		const FLinearColor TitleColor(0.85f, 0.88f, 0.92f);
		const FLinearColor ActiveColor(1.f, 0.84f, 0.47f);      // warm amber (matches Captain label)
		const FLinearColor FilledColor(0.82f, 0.86f, 0.90f);
		const FLinearColor EmptyColor(0.45f, 0.48f, 0.52f);

		float Y = PanelTop;
		for (int32 LineIndex = 0; LineIndex < SlotLines.Num(); ++LineIndex)
		{
			const bool bTitle = (LineIndex == 0);
			const int32 Slot = LineIndex - 1;
			const bool bActive = !bTitle && Party && Slot == ActiveSlot
				&& Party->GetAssignedBattlePlan(Slot) != nullptr;
			const bool bFilled = !bTitle && Party && Party->GetAssignedBattlePlan(Slot) != nullptr;

			FLinearColor Color = TitleColor;
			if (!bTitle)
			{
				Color = bActive ? ActiveColor : (bFilled ? FilledColor : EmptyColor);
			}

			if (bActive)
			{
				FCanvasTileItem Highlight(
					FVector2D(PadX - BoxPad + 2.f, Y - 1.f),
					FVector2D(ContentWidth + BoxPad * 2.f - 4.f, LineHeight + 2.f),
					FLinearColor(0.85f, 0.68f, 0.32f, 0.22f));
				Highlight.BlendMode = SE_BLEND_Translucent;
				Canvas->DrawItem(Highlight);

				FCanvasTextItem MarkerItem(
					FVector2D(PadX, Y), FText::FromString(TEXT(">")), Font, ActiveColor);
				MarkerItem.EnableShadow(FLinearColor(0.f, 0.f, 0.f, 0.9f));
				Canvas->DrawItem(MarkerItem);
			}

			FCanvasTextItem TextItem(
				FVector2D(BodyX, Y), FText::FromString(SlotLines[LineIndex]), Font, Color);
			TextItem.EnableShadow(FLinearColor(0.f, 0.f, 0.f, 0.9f));
			Canvas->DrawItem(TextItem);
			Y += LineHeight + LineGap;
		}

		return Y + BoxPad;
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
	ASolidTerrainStreamer* Streamer = ASolidTerrainStreamer::FindExisting(GetWorld());

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
		int32 CompanionCount = 0;
		if (UWorld* World = GetWorld())
		{
			for (TActorIterator<ASolidCompanionCharacter> It(World); It; ++It)
			{
				ASolidCompanionCharacter* Companion = *It;
				if (!Companion || !Pawn)
				{
					continue;
				}
				++CompanionCount;
				const float Dist = FVector::Dist(Pawn->GetActorLocation(), Companion->GetActorLocation());
				Lines.Add(FString::Printf(
					TEXT("Companion %s  dist=%.0f cm  follow=%.0f"),
					*Companion->GetCharacterDisplayName(),
					Dist,
					Companion->FollowDistance));
			}
		}
		if (CompanionCount == 0)
		{
			Lines.Add(TEXT("Companion <none>"));
		}
	}

	if (const ASolidCharacter* Captain = Cast<ASolidCharacter>(Pawn))
	{
		Lines.Add(FString::Printf(TEXT("F9  %s"), SolidSight::Label(Captain->GetSight())));
	}
	Lines.Add(TEXT("F10  Menu"));

	if (const ASolidCharacter* Captain = Cast<ASolidCharacter>(Pawn))
	{
		if (Captain->IsPartyFormationDrillActive())
		{
			Lines.Add(FString::Printf(
				TEXT("Drill  leg %d/%d  %s %.2fs"),
				Captain->GetPartyFormationDrillLeg() + 1,
				SolidPartyDrill::NumLegs,
				Captain->IsPartyFormationDrillTurning() ? TEXT("turn") : TEXT("walk"),
				Captain->GetPartyFormationDrillPhaseRemaining()));
		}
	}

	UFont* Font = GEngine ? GEngine->GetSmallFont() : nullptr;
	const float BelowTech = SolidHUDPrivate::DrawLines(Canvas, Font, Lines, FLinearColor::White);

	const USolidParty* Party = nullptr;
	if (UWorld* World = GetWorld())
	{
		if (const ASolidGameMode* GameMode = World->GetAuthGameMode<ASolidGameMode>())
		{
			Party = GameMode->GetParty();
		}
	}
	SolidHUDPrivate::DrawBattlePlansPanel(Canvas, Font, Party, BelowTech);

	if (bShowJournal)
	{
		DrawJournalPopup();
	}
	else if (bShowCredits)
	{
		DrawCreditsPopup();
	}
	else if (bShowMainMenu)
	{
		DrawMainMenuPopup();
	}
}

void ASolidHUD::HandleMenuKey()
{
	if (bShowJournal)
	{
		bShowJournal = false;
		return;
	}

	if (bShowCredits)
	{
		bShowCredits = false;
		return;
	}

	bShowMainMenu = !bShowMainMenu;
	if (bShowMainMenu)
	{
		MainMenuIndex = 0;
	}
}

void ASolidHUD::CloseMenuOverlay()
{
	bShowMainMenu = false;
	bShowCredits = false;
	bShowJournal = false;
}

void ASolidHUD::CloseMainMenu()
{
	bShowMainMenu = false;
}

void ASolidHUD::MoveMainMenuSelection(const int32 Delta)
{
	MainMenuIndex = SolidMainMenu::WrapIndex(MainMenuIndex, Delta);
}

void ASolidHUD::SetMainMenuIndex(const int32 Index)
{
	const int32 Count = SolidMainMenu::EntryCount();
	if (Count <= 0)
	{
		MainMenuIndex = 0;
		return;
	}
	MainMenuIndex = FMath::Clamp(Index, 0, Count - 1);
}

void ASolidHUD::ToggleCredits()
{
	bShowCredits = !bShowCredits;
}

void ASolidHUD::DrawCreditsPopup() const
{
	TArray<FString> Lines;
	SolidCredits::CollectLines(Lines);
	SolidHUDPrivate::DrawCenteredPopup(Canvas, Lines, 4.f);
}

void ASolidHUD::DrawJournalPopup() const
{
	SolidEvents::FState State;
	if (UWorld* World = GetWorld())
	{
		if (const ASolidGameMode* GameMode = World->GetAuthGameMode<ASolidGameMode>())
		{
			State = GameMode->GetConfiguration();
		}
	}

	TArray<FString> Lines;
	SolidJournal::CollectLines(State, Lines);
	SolidHUDPrivate::DrawCenteredPopup(Canvas, Lines, 4.f);
}

void ASolidHUD::DrawMainMenuPopup() const
{
	if (!Canvas)
	{
		return;
	}

	UFont* Font = GEngine ? GEngine->GetMediumFont() : nullptr;
	if (!Font && GEngine)
	{
		Font = GEngine->GetSmallFont();
	}

	TArray<FSolidMainMenuEntry> Entries;
	SolidMainMenu::CollectEntries(Entries);

	TArray<FString> Lines;
	Lines.Add(TEXT("Main Menu"));
	Lines.Add(TEXT(""));
	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		const TCHAR* Marker = (Index == MainMenuIndex) ? TEXT(">") : TEXT(" ");
		Lines.Add(FString::Printf(TEXT("%s  %d  %s"), Marker, Index + 1, *Entries[Index].Label));
	}
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("1-3 or Up/Down    Enter    Esc / F10"));

	float MaxWidth = 0.f;
	float LineHeight = 18.f;
	const float LineGap = 6.f;
	for (const FString& Line : Lines)
	{
		float W = 0.f;
		float H = 0.f;
		if (Font)
		{
			Canvas->StrLen(Font, Line.IsEmpty() ? TEXT(" ") : Line, W, H);
		}
		else
		{
			W = static_cast<float>(FMath::Max(Line.Len(), 1) * 8);
			H = 18.f;
		}
		MaxWidth = FMath::Max(MaxWidth, W);
		LineHeight = FMath::Max(LineHeight, H);
	}

	const float BoxPad = 22.f;
	const float BlockHeight = Lines.Num() * LineHeight + (Lines.Num() - 1) * LineGap;
	const float BoxW = MaxWidth + BoxPad * 2.f;
	const float BoxH = BlockHeight + BoxPad * 2.f;
	const float BoxX = (Canvas->SizeX - BoxW) * 0.5f;
	const float BoxY = (Canvas->SizeY - BoxH) * 0.5f;

	FCanvasTileItem Dim(
		FVector2D(0.f, 0.f),
		FVector2D(static_cast<float>(Canvas->SizeX), static_cast<float>(Canvas->SizeY)),
		FLinearColor(0.f, 0.f, 0.f, 0.55f));
	Dim.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Dim);

	FCanvasTileItem Panel(
		FVector2D(BoxX, BoxY),
		FVector2D(BoxW, BoxH),
		FLinearColor(0.06f, 0.07f, 0.09f, 0.92f));
	Panel.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Panel);

	float Y = BoxY + BoxPad;
	for (int32 Index = 0; Index < Lines.Num(); ++Index)
	{
		const FString& Line = Lines[Index];
		FLinearColor Color(0.88f, 0.90f, 0.93f);
		if (Index == 0)
		{
			Color = FLinearColor(1.f, 0.84f, 0.47f);
		}
		else if (Line.StartsWith(TEXT(">")))
		{
			Color = FLinearColor(1.f, 0.84f, 0.47f);
		}
		else if (Line.StartsWith(TEXT("1")))
		{
			Color = FLinearColor(0.55f, 0.58f, 0.62f);
		}

		if (!Line.IsEmpty())
		{
			FCanvasTextItem TextItem(FVector2D(BoxX + BoxPad, Y), FText::FromString(Line), Font, Color);
			TextItem.EnableShadow(FLinearColor(0.f, 0.f, 0.f, 0.85f));
			Canvas->DrawItem(TextItem);
		}
		Y += LineHeight + LineGap;
	}
}
