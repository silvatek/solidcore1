#include "SolidHUD.h"
#include "HUD/SolidHudTheme.h"
#include "HUD/SolidPointer.h"
#include "Strongholds/SolidDungeonPassage.h"
#include "SolidBuildId.h"
#include "Menus/SolidCharacterEditor.h"
#include "SolidBodyParams.h"
#include "Menus/SolidCredits.h"
#include "Inventory/SolidInventory.h"
#include "Inventory/SolidStorage.h"
#include "Strongholds/SolidStrongholdFurniture.h"
#include "Strongholds/SolidStrongholdTransition.h"
#include "Journal/SolidJournal.h"
#include "Menus/SolidMainMenu.h"
#include "Menus/SolidMenuPointer.h"
#include "Menus/SolidSquadBuilder.h"
#include "Menus/SolidExit.h"
#include "Menus/SolidPortal.h"
#include "SolidCharacter.h"
#include "SolidSight.h"
#include "SolidGameMode.h"
#include "SolidGameInstance.h"
#include "SolidPlayerController.h"
#include "Companion/SolidCompanionCharacter.h"
#include "Squad/SolidBattlePlan.h"
#include "Squad/SolidSquad.h"
#include "Squad/SolidSquadDrill.h"
#include "Terrain/SolidTerrainMap.h"
#include "Terrain/SolidTerrainStreamer.h"
#include "Terrain/SolidTerrainTypes.h"
#include "GameFramework/GameModeBase.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Engine/SkeletalMesh.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInterface.h"
#include "Fonts/CompositeFont.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateBrush.h"
#include "Styling/StyleDefaults.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Misc/Paths.h"

namespace SolidHUDPrivate
{
	static void FillRect(
		UCanvas* Canvas,
		const float X,
		const float Y,
		const float W,
		const float H,
		const FLinearColor& Color)
	{
		if (!Canvas || W <= 0.f || H <= 0.f)
		{
			return;
		}
		FCanvasTileItem Tile(FVector2D(X, Y), FVector2D(W, H), Color);
		Tile.BlendMode = SE_BLEND_Opaque;
		Canvas->DrawItem(Tile);
	}

	/** Dark oak board, bronze binding, brighter bosses on the corners. */
	static void DrawOakFrame(
		UCanvas* Canvas,
		const float X,
		const float Y,
		const float W,
		const float H,
		const SolidHudTheme::FFrame& Frame)
	{
		const SolidHudTheme::FPalette Palette = SolidHudTheme::OakAndBronze();
		float InnerW = 0.f;
		float InnerH = 0.f;
		if (!SolidHudTheme::InnerSize(W, H, Frame, InnerW, InnerH))
		{
			FillRect(Canvas, X, Y, W, H, Palette.Fill);
			return;
		}

		FillRect(Canvas, X, Y, W, H, Palette.Bronze);
		const float Inset1 = Frame.Outer;
		FillRect(Canvas, X + Inset1, Y + Inset1, W - Inset1 * 2.f, H - Inset1 * 2.f, Palette.Recess);
		const float Inset2 = Inset1 + Frame.Gap;
		FillRect(Canvas, X + Inset2, Y + Inset2, W - Inset2 * 2.f, H - Inset2 * 2.f, Palette.Bronze);
		FillRect(Canvas, X + Inset2 + Frame.Inner, Y + Inset2 + Frame.Inner, InnerW, InnerH, Palette.Fill);

		const float Corner = FMath::Min(Frame.Corner, FMath::Min(W, H) * 0.5f);
		FillRect(Canvas, X, Y, Corner, Corner, Palette.BronzeBright);
		FillRect(Canvas, X + W - Corner, Y, Corner, Corner, Palette.BronzeBright);
		FillRect(Canvas, X, Y + H - Corner, Corner, Corner, Palette.BronzeBright);
		FillRect(Canvas, X + W - Corner, Y + H - Corner, Corner, Corner, Palette.BronzeBright);
	}

	static void DrawPointerTriangle(
		UCanvas* Canvas,
		const SolidPointer::FTriangle& Screen,
		const FLinearColor& Color)
	{
		const float MinY = FMath::Min3(Screen.A.Y, Screen.B.Y, Screen.C.Y);
		const float MaxY = FMath::Max3(Screen.A.Y, Screen.B.Y, Screen.C.Y);
		const int32 FirstRow = FMath::FloorToInt(MinY);
		const int32 LastRow = FMath::FloorToInt(MaxY);
		for (int32 Row = FirstRow; Row <= LastRow; ++Row)
		{
			float Left = 0.f;
			float Right = 0.f;
			if (!SolidPointer::SpanAtY(Screen, static_cast<float>(Row) + 0.5f, Left, Right))
			{
				continue;
			}
			FillRect(Canvas, Left, static_cast<float>(Row), FMath::Max(1.f, Right - Left), 1.f, Color);
		}
	}

	static void DrawPointerLayer(
		UCanvas* Canvas,
		const FVector2D& Hotspot,
		const FLinearColor& Color)
	{
		for (int32 Index = 0; Index < SolidPointer::TriangleCount; ++Index)
		{
			DrawPointerTriangle(
				Canvas,
				SolidPointer::Place(SolidPointer::Triangle(Index), Hotspot),
				Color);
		}
	}

	/** Bronze arrow. The tip is the hotspot. Drawn last so it sits on the panels. */
	static void DrawPointer(UCanvas* Canvas, const FVector2D& Hotspot)
	{
		if (!Canvas)
		{
			return;
		}
		const SolidHudTheme::FPalette Palette = SolidHudTheme::OakAndBronze();
		FVector2D Rim[SolidPointer::OutlineOffsetCount];
		SolidPointer::OutlineOffsets(SolidPointer::OutlinePad, Rim);
		for (const FVector2D& Offset : Rim)
		{
			DrawPointerLayer(Canvas, Hotspot + Offset, Palette.Recess);
		}
		FVector2D Edge[SolidPointer::OutlineOffsetCount];
		SolidPointer::OutlineOffsets(SolidPointer::EdgePad, Edge);
		for (const FVector2D& Offset : Edge)
		{
			DrawPointerLayer(Canvas, Hotspot + Offset, Palette.Bronze);
		}
		DrawPointerLayer(Canvas, Hotspot, Palette.BronzeBright);
	}

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

		DrawOakFrame(Canvas, BoxX, BoxY, BoxW, BoxH, SolidHudTheme::MenuFrame());

		const SolidHudTheme::FPalette Palette = SolidHudTheme::OakAndBronze();
		float Y = BoxY + BoxPad;
		for (int32 Index = 0; Index < Lines.Num(); ++Index)
		{
			const FString& Line = Lines[Index];
			FLinearColor Color = Palette.Ink;
			if (Index == 0)
			{
				Color = Palette.Title;
			}
			else if (Line.StartsWith(TEXT("Fab assets")) || Line.StartsWith(TEXT("SolidCore1"))
				|| Line.StartsWith(TEXT("F10")))
			{
				Color = Palette.Hint;
			}

			if (Index == 0 && !Line.IsEmpty())
			{
				const float RuleInset = SolidHudTheme::BandThickness(SolidHudTheme::MenuFrame()) + 18.f;
				FillRect(
					Canvas,
					BoxX + RuleInset,
					Y + LineHeight + LineGap * 0.35f,
					FMath::Max(0.f, BoxW - RuleInset * 2.f),
					2.f,
					Palette.Bronze);
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
		const USolidSquad* Squad,
		const float StartY)
	{
		if (!Canvas)
		{
			return StartY;
		}

		const float PadX = 16.f;
		const float LineGap = 2.f;
		const SolidHudTheme::FFrame Frame = SolidHudTheme::HudFrame();
		const float BoxPad = FMath::Max(14.f, SolidHudTheme::BandThickness(Frame) + 8.f);
		const float GapAbove = 10.f;
		const float PanelTop = StartY + GapAbove;

		const int32 ActiveSlot = Squad ? Squad->GetActiveAssignedSlot() : INDEX_NONE;
		const int32 SlotCount = SolidBattleFormationSlots::MaxAssignedBattlePlans;

		TArray<FString> SlotLines;
		SlotLines.Reserve(SlotCount + 1);
		SlotLines.Add(TEXT("Battle Plans"));
		for (int32 Slot = 0; Slot < SlotCount; ++Slot)
		{
			const FSolidBattlePlan* Plan = Squad ? Squad->GetAssignedBattlePlan(Slot) : nullptr;
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
		const float BoxX = PadX - BoxPad;
		const float BoxY = PanelTop - BoxPad * 0.5f;
		const float BoxW = ContentWidth + BoxPad * 2.f;
		const float BoxH = BlockHeight + BoxPad;
		DrawOakFrame(Canvas, BoxX, BoxY, BoxW, BoxH, Frame);

		const SolidHudTheme::FPalette Palette = SolidHudTheme::OakAndBronze();
		const FLinearColor TitleColor = Palette.Title;
		const FLinearColor ActiveColor = Palette.Title;
		const FLinearColor FilledColor = Palette.Ink;
		const FLinearColor EmptyColor = Palette.Hint;

		float Y = PanelTop;
		for (int32 LineIndex = 0; LineIndex < SlotLines.Num(); ++LineIndex)
		{
			const bool bTitle = (LineIndex == 0);
			const int32 Slot = LineIndex - 1;
			const bool bActive = !bTitle && Squad && Slot == ActiveSlot
				&& Squad->GetAssignedBattlePlan(Slot) != nullptr;
			const bool bFilled = !bTitle && Squad && Squad->GetAssignedBattlePlan(Slot) != nullptr;

			FLinearColor Color = TitleColor;
			if (!bTitle)
			{
				Color = bActive ? ActiveColor : (bFilled ? FilledColor : EmptyColor);
			}

			if (bActive)
			{
				const float Band = SolidHudTheme::BandThickness(Frame);
				FCanvasTileItem Highlight(
					FVector2D(BoxX + Band + 3.f, Y - 1.f),
					FVector2D(FMath::Max(0.f, BoxW - (Band + 3.f) * 2.f), LineHeight + 2.f),
					FLinearColor(Palette.Bronze.R, Palette.Bronze.G, Palette.Bronze.B, 0.35f));
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

	/** Roboto has no emoji, so the marker uses Segoe UI Emoji when that file is present. */
	static FSlateFontInfo SightMarkerFont()
	{
		static TSharedPtr<const FCompositeFont> EmojiFont;
		if (!EmojiFont.IsValid())
		{
			const FString SystemRoot = FPlatformMisc::GetEnvironmentVariable(TEXT("SystemRoot"));
			const FString FontPath = FPaths::Combine(SystemRoot, TEXT("Fonts"), TEXT("seguiemj.ttf"));
			if (!SystemRoot.IsEmpty() && FPaths::FileExists(FontPath))
			{
				const TSharedRef<FCompositeFont> Composite = MakeShared<FCompositeFont>();
				FTypefaceEntry& Entry = Composite->DefaultTypeface.Fonts.AddDefaulted_GetRef();
				Entry.Name = FName(TEXT("Regular"));
				Entry.Font = FFontData(FontPath, EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
				EmojiFont = Composite;
			}
		}

		FSlateFontInfo Font = EmojiFont.IsValid()
			? FSlateFontInfo(EmojiFont.ToSharedRef(), 28.f, FName(TEXT("Regular")))
			: FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 28);
		return Font;
	}

	static const FSlateBrush* SightMarkerBrush()
	{
		const SolidHudTheme::FSightMarkerChip Chip = SolidHudTheme::SightMarkerChip();
		static const FSlateRoundedBoxBrush Brush(Chip.Fill, Chip.CornerRadius);
		return &Brush;
	}

	static TMap<ASolidHUD*, TSharedPtr<SWidget>> SightIndicators;
	static TMap<ASolidHUD*, TArray<SolidMenuPointer::FRow>> ChoicePointerRows;

	static void ForgetChoicePointerRows(ASolidHUD* HUD)
	{
		ChoicePointerRows.Remove(HUD);
	}

	static void DrawChoicePopup(
		UCanvas* Canvas,
		const FString& Title,
		const TArray<FString>& ChoiceLabels,
		const int32 SelectedIndex,
		TArray<SolidMenuPointer::FRow>& OutRows,
		const TArray<bool>* ChoiceEnabled = nullptr)
	{
		OutRows.Reset();
		if (!Canvas)
		{
			return;
		}

		UFont* Font = GEngine ? GEngine->GetMediumFont() : nullptr;
		if (!Font && GEngine)
		{
			Font = GEngine->GetSmallFont();
		}

		TArray<FString> Lines;
		Lines.Add(Title);
		Lines.Add(TEXT(""));
		for (int32 Index = 0; Index < ChoiceLabels.Num(); ++Index)
		{
			const TCHAR* Marker = (Index == SelectedIndex) ? TEXT(">") : TEXT(" ");
			Lines.Add(FString::Printf(TEXT("%s  %d  %s"), Marker, Index + 1, *ChoiceLabels[Index]));
		}
		Lines.Add(TEXT(""));
		Lines.Add(FString::Printf(
			TEXT("1-%d, click, Up/Down, Enter, Esc / F10"),
			FMath::Max(ChoiceLabels.Num(), 1)));

		float MaxWidth = 0.f;
		float LineHeight = SolidMenuPointer::FallbackLineHeight;
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
				H = SolidMenuPointer::FallbackLineHeight;
			}
			MaxWidth = FMath::Max(MaxWidth, W);
			LineHeight = FMath::Max(LineHeight, H);
		}

		SolidMenuPointer::FMetrics Metrics;
		Metrics.CanvasW = static_cast<float>(Canvas->SizeX);
		Metrics.CanvasH = static_cast<float>(Canvas->SizeY);
		Metrics.ContentW = MaxWidth;
		Metrics.LineHeight = LineHeight;
		Metrics.ChoiceCount = ChoiceLabels.Num();
		float BoxX = 0.f;
		float BoxY = 0.f;
		float BoxW = 0.f;
		float BoxH = 0.f;
		SolidMenuPointer::PanelBox(Metrics, BoxX, BoxY, BoxW, BoxH);
		SolidMenuPointer::ChoiceRows(Metrics, OutRows);
		(void)BoxH;

		FCanvasTileItem Dim(
			FVector2D(0.f, 0.f),
			FVector2D(static_cast<float>(Canvas->SizeX), static_cast<float>(Canvas->SizeY)),
			FLinearColor(0.f, 0.f, 0.f, 0.72f));
		Dim.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(Dim);

		DrawOakFrame(Canvas, BoxX, BoxY, BoxW, BoxH, SolidHudTheme::MenuFrame());

		const SolidHudTheme::FPalette Palette = SolidHudTheme::OakAndBronze();
		for (int32 Index = 0; Index < Lines.Num(); ++Index)
		{
			const FString& Line = Lines[Index];
			const float Y = SolidMenuPointer::LineY(BoxY, Metrics, Index);
			const int32 ChoiceIndex = Index - SolidMenuPointer::LeadingLines;
			const bool bChoiceLine = ChoiceIndex >= 0 && ChoiceIndex < ChoiceLabels.Num();
			const bool bChoiceEnabled = !bChoiceLine
				|| !ChoiceEnabled
				|| !ChoiceEnabled->IsValidIndex(ChoiceIndex)
				|| (*ChoiceEnabled)[ChoiceIndex];
			FLinearColor Color = Palette.Ink;
			if (!bChoiceEnabled)
			{
				Color = FLinearColor(Palette.Ink.R, Palette.Ink.G, Palette.Ink.B, 0.4f);
			}
			else if (Index == 0 || Line.StartsWith(TEXT(">")))
			{
				Color = Palette.Title;
			}
			else if (Line.StartsWith(TEXT("1")))
			{
				Color = Palette.Hint;
			}

			if (Index == 0 && !Line.IsEmpty())
			{
				const float RuleInset = SolidHudTheme::BandThickness(SolidHudTheme::MenuFrame()) + 18.f;
				FillRect(
					Canvas,
					BoxX + RuleInset,
					Y + LineHeight + Metrics.LineGap * 0.35f,
					FMath::Max(0.f, BoxW - RuleInset * 2.f),
					2.f,
					Palette.Bronze);
			}

			if (!Line.IsEmpty())
			{
				FCanvasTextItem TextItem(FVector2D(BoxX + Metrics.BoxPad, Y), FText::FromString(Line), Font, Color);
				TextItem.EnableShadow(FLinearColor(0.f, 0.f, 0.f, 0.85f));
				Canvas->DrawItem(TextItem);
			}
		}
	}

	/** Large journal board. Time and the entry are separate columns. The close hint stays at the bottom. */
	static void DrawJournalBoard(UCanvas* Canvas, const TArray<SolidJournal::FRow>& Rows)
	{
		if (!Canvas || Rows.Num() == 0)
		{
			return;
		}

		UFont* Font = GEngine ? GEngine->GetMediumFont() : nullptr;
		if (!Font && GEngine)
		{
			Font = GEngine->GetSmallFont();
		}

		float StampWidth = 0.f;
		float TextWidth = 0.f;
		float LineHeight = 18.f;
		float TitleW = 0.f;
		float TitleH = 18.f;
		FString TitleText(SolidJournal::Title);
		int32 FlowRows = 0;
		for (const SolidJournal::FRow& Row : Rows)
		{
			if (Row.Kind == SolidJournal::ERowKind::Title)
			{
				if (!Row.Text.IsEmpty())
				{
					TitleText = Row.Text;
				}
				continue;
			}
			if (Row.Kind != SolidJournal::ERowKind::Hint)
			{
				++FlowRows;
			}

			float StampW = 0.f;
			float StampH = 0.f;
			float TextW = 0.f;
			float TextH = 0.f;
			if (Font)
			{
				if (!Row.Stamp.IsEmpty())
				{
					Canvas->StrLen(Font, Row.Stamp, StampW, StampH);
				}
				Canvas->StrLen(Font, Row.Text.IsEmpty() ? TEXT(" ") : Row.Text, TextW, TextH);
			}
			else
			{
				StampW = static_cast<float>(Row.Stamp.Len() * 8);
				TextW = static_cast<float>(FMath::Max(Row.Text.Len(), 1) * 8);
				StampH = 18.f;
				TextH = 18.f;
			}
			StampWidth = FMath::Max(StampWidth, StampW);
			TextWidth = FMath::Max(TextWidth, TextW);
			LineHeight = FMath::Max(LineHeight, FMath::Max(StampH, TextH));
		}
		if (Font)
		{
			Canvas->StrLen(Font, TitleText, TitleW, TitleH);
		}
		else
		{
			TitleW = static_cast<float>(TitleText.Len() * 8);
			TitleH = 18.f;
		}

		const float ColumnSpan = StampWidth > 0.f ? StampWidth + SolidJournal::ColumnGap : 0.f;
		const float ContentW = ColumnSpan + TextWidth;
		const float ContentH = static_cast<float>(FlowRows) * (LineHeight + SolidJournal::LineGap) + LineHeight;
		float BoardW = 0.f;
		float BoardH = 0.f;
		SolidJournal::BoardSize(
			static_cast<float>(Canvas->SizeX),
			static_cast<float>(Canvas->SizeY),
			ContentW,
			ContentH,
			BoardW,
			BoardH);

		const float BoxX = (static_cast<float>(Canvas->SizeX) - BoardW) * 0.5f;
		const float BoxY = (static_cast<float>(Canvas->SizeY) - BoardH) * 0.5f;

		FCanvasTileItem Dim(
			FVector2D(0.f, 0.f),
			FVector2D(static_cast<float>(Canvas->SizeX), static_cast<float>(Canvas->SizeY)),
			FLinearColor(0.f, 0.f, 0.f, 0.72f));
		Dim.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(Dim);

		const SolidHudTheme::FFrame Frame = SolidHudTheme::MenuFrame();
		DrawOakFrame(Canvas, BoxX, BoxY, BoardW, BoardH, Frame);

		const SolidHudTheme::FPalette Palette = SolidHudTheme::OakAndBronze();
		const float Band = SolidHudTheme::BandThickness(Frame);
		const SolidJournal::FTitlePlate Plate = SolidJournal::TitlePlate(BoxX, BoxY, BoardW, TitleW, TitleH, Band);
		FillRect(Canvas, Plate.X, Plate.Y, Plate.W, Plate.H, Palette.Bronze);
		if (!TitleText.IsEmpty())
		{
			const float NameX = Plate.X + (Plate.W - TitleW) * 0.5f;
			const float NameY = Plate.Y + (Plate.H - TitleH) * 0.5f;
			FCanvasTextItem NameItem(FVector2D(NameX, NameY), FText::FromString(TitleText), Font, Palette.Title);
			NameItem.EnableShadow(FLinearColor(0.f, 0.f, 0.f, 0.85f));
			Canvas->DrawItem(NameItem);
		}

		const float OriginX = BoxX + SolidJournal::BoardPad;
		const SolidJournal::FColumns Columns = SolidJournal::Columns(OriginX, StampWidth);
		const float HintY = BoxY + BoardH - SolidJournal::BoardPad - LineHeight;
		float Y = BoxY + SolidJournal::BoardPad;

		for (const SolidJournal::FRow& Row : Rows)
		{
			if (Row.Kind == SolidJournal::ERowKind::Title || Row.Kind == SolidJournal::ERowKind::Hint)
			{
				continue;
			}
			// Leave the close hint. A scrollbar will cover lines that do not fit.
			if (Y + LineHeight > HintY)
			{
				break;
			}

			const bool bHeading = Row.Kind == SolidJournal::ERowKind::Heading;
			const FLinearColor Color = bHeading ? Palette.Title : Palette.Ink;

			if (!Row.Stamp.IsEmpty())
			{
				FCanvasTextItem StampItem(
					FVector2D(Columns.StampX, Y), FText::FromString(Row.Stamp), Font, Color);
				StampItem.EnableShadow(FLinearColor(0.f, 0.f, 0.f, 0.85f));
				Canvas->DrawItem(StampItem);
			}
			if (!Row.Text.IsEmpty())
			{
				FCanvasTextItem TextItem(
					FVector2D(Columns.TextX, Y), FText::FromString(Row.Text), Font, Color);
				TextItem.EnableShadow(FLinearColor(0.f, 0.f, 0.f, 0.85f));
				Canvas->DrawItem(TextItem);
			}

			if (bHeading)
			{
				const float RuleInset = Band + 18.f;
				FillRect(
					Canvas,
					BoxX + RuleInset,
					Y + LineHeight + SolidJournal::LineGap * 0.35f,
					FMath::Max(0.f, BoardW - RuleInset * 2.f),
					2.f,
					Palette.Bronze);
			}

			Y += LineHeight + SolidJournal::LineGap;
		}

		for (const SolidJournal::FRow& Row : Rows)
		{
			if (Row.Kind != SolidJournal::ERowKind::Hint || Row.Text.IsEmpty())
			{
				continue;
			}
			FCanvasTextItem HintItem(
				FVector2D(OriginX, HintY), FText::FromString(Row.Text), Font, Palette.Hint);
			HintItem.EnableShadow(FLinearColor(0.f, 0.f, 0.f, 0.85f));
			Canvas->DrawItem(HintItem);
			break;
		}
	}

	static void DrawSlotBox(
		UCanvas* Canvas,
		const float X,
		const float Y,
		const float W,
		const float H)
	{
		const SolidHudTheme::FPalette Palette = SolidHudTheme::OakAndBronze();
		const float Rim = 2.f;
		FillRect(Canvas, X, Y, W, H, Palette.Bronze);
		FillRect(Canvas, X + Rim, Y + Rim, FMath::Max(0.f, W - Rim * 2.f), FMath::Max(0.f, H - Rim * 2.f), Palette.Recess);
	}

	static void DrawCenteredLine(
		UCanvas* Canvas,
		UFont* Font,
		const FString& Text,
		const float BoxX,
		const float BoxY,
		const float BoxW,
		const float BoxH,
		const FLinearColor& Color)
	{
		if (!Canvas || !Font || Text.IsEmpty() || BoxW <= 0.f || BoxH <= 0.f)
		{
			return;
		}
		float TextW = 0.f;
		float TextH = 0.f;
		Canvas->StrLen(Font, Text, TextW, TextH);
		FCanvasTextItem Item(
			FVector2D(BoxX + (BoxW - TextW) * 0.5f, BoxY + (BoxH - TextH) * 0.5f),
			FText::FromString(Text),
			Font,
			Color);
		Item.EnableShadow(FLinearColor(0.f, 0.f, 0.f, 0.85f));
		Canvas->DrawItem(Item);
	}

	/** Pack, Chest, and a worn-slot name. Centered on the width, sitting on LabelY. */
	static void DrawInventoryHeading(
		UCanvas* Canvas,
		UFont* Font,
		const FString& Text,
		const float Left,
		const float Width,
		const float LabelY,
		const FLinearColor& Color)
	{
		if (!Canvas || !Font || Text.IsEmpty() || Width <= 0.f)
		{
			return;
		}
		float TextW = 0.f;
		float TextH = 0.f;
		Canvas->StrLen(Font, Text, TextW, TextH);
		FCanvasTextItem Item(
			FVector2D(Left + (Width - TextW) * 0.5f, LabelY),
			FText::FromString(Text),
			Font,
			Color);
		Item.EnableShadow(FLinearColor(0.f, 0.f, 0.f, 0.85f));
		Canvas->DrawItem(Item);
	}

	/** Same oak board as the journal. Worn boxes follow a person. The pack is a grid on the right. */
	static void DrawInventoryBoard(UCanvas* Canvas, const SolidInventory::FPage& Page)
	{
		if (!Canvas)
		{
			return;
		}

		UFont* TitleFont = GEngine ? GEngine->GetMediumFont() : nullptr;
		UFont* Font = GEngine ? GEngine->GetSmallFont() : nullptr;
		if (!TitleFont)
		{
			TitleFont = Font;
		}
		if (!Font)
		{
			Font = TitleFont;
		}

		const FString TitleText(SolidInventory::Title);
		float TitleW = static_cast<float>(TitleText.Len() * 8);
		float TitleH = 18.f;
		float LineHeight = 16.f;
		if (TitleFont)
		{
			Canvas->StrLen(TitleFont, TitleText, TitleW, TitleH);
		}
		if (Font)
		{
			float HintW = 0.f;
			Canvas->StrLen(Font, SolidInventory::CloseHint, HintW, LineHeight);
		}

		float BoardW = 0.f;
		float BoardH = 0.f;
		SolidJournal::BoardSize(
			static_cast<float>(Canvas->SizeX),
			static_cast<float>(Canvas->SizeY),
			Page.ContentW,
			Page.ContentH + LineHeight + SolidJournal::LineGap,
			BoardW,
			BoardH);

		const float BoxX = (static_cast<float>(Canvas->SizeX) - BoardW) * 0.5f;
		const float BoxY = (static_cast<float>(Canvas->SizeY) - BoardH) * 0.5f;
		const SolidInventory::FView View = SolidInventory::ContentOrigin(
			static_cast<float>(Canvas->SizeX),
			static_cast<float>(Canvas->SizeY),
			Page,
			LineHeight);

		FCanvasTileItem Dim(
			FVector2D(0.f, 0.f),
			FVector2D(static_cast<float>(Canvas->SizeX), static_cast<float>(Canvas->SizeY)),
			FLinearColor(0.f, 0.f, 0.f, 0.72f));
		Dim.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(Dim);

		const SolidHudTheme::FFrame Frame = SolidHudTheme::MenuFrame();
		DrawOakFrame(Canvas, BoxX, BoxY, BoardW, BoardH, Frame);

		const SolidHudTheme::FPalette Palette = SolidHudTheme::OakAndBronze();
		const float Band = SolidHudTheme::BandThickness(Frame);
		const SolidJournal::FTitlePlate Plate = SolidJournal::TitlePlate(BoxX, BoxY, BoardW, TitleW, TitleH, Band);
		FillRect(Canvas, Plate.X, Plate.Y, Plate.W, Plate.H, Palette.Bronze);
		if (TitleFont)
		{
			const float NameX = Plate.X + (Plate.W - TitleW) * 0.5f;
			const float NameY = Plate.Y + (Plate.H - TitleH) * 0.5f;
			FCanvasTextItem NameItem(FVector2D(NameX, NameY), FText::FromString(TitleText), TitleFont, Palette.Title);
			NameItem.EnableShadow(FLinearColor(0.f, 0.f, 0.f, 0.85f));
			Canvas->DrawItem(NameItem);
		}

		const float OriginX = View.OriginX;
		const float OriginY = View.OriginY;
		DrawInventoryHeading(
			Canvas,
			Font,
			SolidInventory::PackTitle,
			OriginX + Page.PackX,
			Page.PackGridW,
			OriginY + Page.PackLabelY,
			Palette.Title);
		if (Page.bShowChest)
		{
			DrawInventoryHeading(
				Canvas,
				Font,
				SolidStorage::ChestTitle,
				OriginX + Page.ChestX,
				Page.ChestGridW,
				OriginY + Page.ChestLabelY,
				Palette.Title);
		}

		for (const SolidInventory::FBox& Slot : Page.Boxes)
		{
			const float X = OriginX + Slot.X;
			const float Y = OriginY + Slot.Y;
			DrawSlotBox(Canvas, X, Y, Slot.W, Slot.H);
			if (!Font)
			{
				continue;
			}
			if (!Slot.bPack && !Slot.bChest && !Slot.bCraft)
			{
				DrawInventoryHeading(
					Canvas,
					Font,
					Slot.Label,
					X,
					Slot.W,
					OriginY + Slot.LabelY,
					Palette.Title);
			}
			if (!Slot.Item.IsEmpty())
			{
				DrawCenteredLine(Canvas, Font, Slot.Item, X, Y, Slot.W, Slot.H, Palette.Ink);
			}
		}

		if (Page.CraftButtonW > 0.f && Page.CraftButtonH > 0.f)
		{
			const float ButtonX = OriginX + Page.CraftButtonX;
			const float ButtonY = OriginY + Page.CraftButtonY;
			// A solid bronze bar. Slots are a dark well inside a rim; this is the metal itself.
			const FLinearColor Face = Page.bCraftEnabled
				? Palette.Bronze
				: FLinearColor(Palette.Bronze.R * 0.55f, Palette.Bronze.G * 0.55f, Palette.Bronze.B * 0.55f, 1.f);
			const FLinearColor Edge = Page.bCraftEnabled
				? Palette.BronzeBright
				: FLinearColor(Palette.Bronze.R * 0.35f, Palette.Bronze.G * 0.35f, Palette.Bronze.B * 0.35f, 1.f);
			const FLinearColor Ink = Page.bCraftEnabled ? Palette.Fill : Palette.Hint;
			const float Rim = 2.f;
			FillRect(Canvas, ButtonX, ButtonY, Page.CraftButtonW, Page.CraftButtonH, Edge);
			FillRect(
				Canvas,
				ButtonX + Rim,
				ButtonY + Rim,
				FMath::Max(0.f, Page.CraftButtonW - Rim * 2.f),
				FMath::Max(0.f, Page.CraftButtonH - Rim * 2.f),
				Face);
			if (Font)
			{
				DrawCenteredLine(
					Canvas,
					Font,
					SolidInventory::CraftButtonLabel,
					ButtonX,
					ButtonY,
					Page.CraftButtonW,
					Page.CraftButtonH,
					Ink);
			}
		}

		if (Font)
		{
			const float HintY = BoxY + BoardH - SolidJournal::BoardPad - LineHeight;
			FCanvasTextItem HintItem(
				FVector2D(BoxX + SolidJournal::BoardPad, HintY),
				FText::FromString(SolidInventory::CloseHint),
				Font,
				Palette.Hint);
			HintItem.EnableShadow(FLinearColor(0.f, 0.f, 0.f, 0.85f));
			Canvas->DrawItem(HintItem);
		}
	}

	static void DrawPageHeading(
		UCanvas* Canvas,
		UFont* Font,
		const FString& Text,
		const SolidSquadBuilder::FRect& Rect,
		const float OriginX,
		const float OriginY,
		const FLinearColor& Color)
	{
		if (!Canvas || !Font || Text.IsEmpty())
		{
			return;
		}
		FCanvasTextItem Item(
			FVector2D(OriginX + Rect.X, OriginY + Rect.Y),
			FText::FromString(Text),
			Font,
			Color);
		Item.EnableShadow(FLinearColor(0.f, 0.f, 0.f, 0.85f));
		Canvas->DrawItem(Item);
	}

	static void DrawSquadThumb(
		UCanvas* Canvas,
		const bool bShow,
		const SolidSquadBuilder::FRect& Thumb,
		const float OriginX,
		const float OriginY,
		const FLinearColor& Color)
	{
		if (!Canvas || !bShow)
		{
			return;
		}
		FillRect(Canvas, OriginX + Thumb.X, OriginY + Thumb.Y, Thumb.W, Thumb.H, Color);
	}

	static void DrawSquadPage(UCanvas* Canvas, const SolidSquadBuilder::FPage& Page, const float OriginX, const float OriginY)
	{
		if (!Canvas)
		{
			return;
		}
		UFont* Font = GEngine ? GEngine->GetSmallFont() : nullptr;
		const SolidHudTheme::FPalette Palette = SolidHudTheme::OakAndBronze();

		DrawPageHeading(Canvas, Font, TEXT("Company"), Page.CompanyHeading, OriginX, OriginY, Palette.Title);
		DrawPageHeading(Canvas, Font, TEXT("Squad"), Page.SquadHeading, OriginX, OriginY, Palette.Title);
		DrawPageHeading(Canvas, Font, TEXT("Plans"), Page.PlansHeading, OriginX, OriginY, Palette.Title);

		for (int32 Index = 0; Index < Page.VocationChips.Num(); ++Index)
		{
			const SolidSquadBuilder::FRect& Chip = Page.VocationChips[Index];
			const float X = OriginX + Chip.X;
			const float Y = OriginY + Chip.Y;
			const bool bSelected = static_cast<int32>(Page.Vocation) == Index;
			if (bSelected)
			{
				FillRect(Canvas, X, Y, Chip.W, Chip.H, Palette.Bronze);
			}
			else
			{
				DrawSlotBox(Canvas, X, Y, Chip.W, Chip.H);
			}
			DrawCenteredLine(
				Canvas,
				Font,
				SolidSquadBuilder::VocationFilterName(static_cast<SolidSquadBuilder::EVocationFilter>(Index)),
				X,
				Y,
				Chip.W,
				Chip.H,
				bSelected ? Palette.Fill : Palette.Ink);
		}
		for (int32 Index = 0; Index < Page.FormationChips.Num(); ++Index)
		{
			const SolidSquadBuilder::FRect& Chip = Page.FormationChips[Index];
			const float X = OriginX + Chip.X;
			const float Y = OriginY + Chip.Y;
			const bool bSelected = static_cast<int32>(Page.Formation) == Index;
			if (bSelected)
			{
				FillRect(Canvas, X, Y, Chip.W, Chip.H, Palette.Bronze);
			}
			else
			{
				DrawSlotBox(Canvas, X, Y, Chip.W, Chip.H);
			}
			DrawCenteredLine(
				Canvas,
				Font,
				SolidSquadBuilder::FormationFilterName(static_cast<SolidSquadBuilder::EFormationFilter>(Index)),
				X,
				Y,
				Chip.W,
				Chip.H,
				bSelected ? Palette.Fill : Palette.Ink);
		}

		DrawSlotBox(
			Canvas,
			OriginX + Page.CompanyBox.X,
			OriginY + Page.CompanyBox.Y,
			Page.CompanyBox.W,
			Page.CompanyBox.H);
		DrawSlotBox(
			Canvas,
			OriginX + Page.SquadFrame.X,
			OriginY + Page.SquadFrame.Y,
			Page.SquadFrame.W,
			Page.SquadFrame.H);
		DrawSlotBox(
			Canvas,
			OriginX + Page.PlansBox.X,
			OriginY + Page.PlansBox.Y,
			Page.PlansBox.W,
			Page.PlansBox.H);

		for (const SolidSquadBuilder::FEntry& Row : Page.CompanyRows)
		{
			if (!Row.bVisible)
			{
				continue;
			}
			DrawCenteredLine(
				Canvas,
				Font,
				Row.Label,
				OriginX + Row.Rect.X,
				OriginY + Row.Rect.Y,
				Row.Rect.W,
				Row.Rect.H,
				Palette.Ink);
		}
		for (const SolidSquadBuilder::FEntry& Box : Page.SquadSlots)
		{
			if (!Box.bVisible)
			{
				continue;
			}
			DrawSlotBox(Canvas, OriginX + Box.Rect.X, OriginY + Box.Rect.Y, Box.Rect.W, Box.Rect.H);
			DrawCenteredLine(
				Canvas,
				Font,
				Box.Label,
				OriginX + Box.Rect.X,
				OriginY + Box.Rect.Y,
				Box.Rect.W,
				Box.Rect.H,
				Palette.Ink);
		}
		for (const SolidSquadBuilder::FEntry& Row : Page.PlanRows)
		{
			if (!Row.bVisible)
			{
				continue;
			}
			DrawCenteredLine(
				Canvas,
				Font,
				Row.Label,
				OriginX + Row.Rect.X,
				OriginY + Row.Rect.Y,
				Row.Rect.W,
				Row.Rect.H,
				Palette.Ink);
		}
		for (int32 Slot = 0; Slot < Page.Keys.Num(); ++Slot)
		{
			const SolidSquadBuilder::FEntry& Key = Page.Keys[Slot];
			const float X = OriginX + Key.Rect.X;
			const float Y = OriginY + Key.Rect.Y;
			DrawSlotBox(Canvas, X, Y, Key.Rect.W, Key.Rect.H);
			const FString Text = Key.Label.IsEmpty()
				? FString::Printf(TEXT("F%d"), Slot + 1)
				: FString::Printf(TEXT("F%d  %s"), Slot + 1, *Key.Label);
			DrawCenteredLine(Canvas, Font, Text, X, Y, Key.Rect.W, Key.Rect.H, Palette.Ink);
		}

		DrawSquadThumb(Canvas, Page.bCompanyThumb, Page.CompanyThumb, OriginX, OriginY, Palette.BronzeBright);
		DrawSquadThumb(Canvas, Page.bSquadThumb, Page.SquadThumb, OriginX, OriginY, Palette.BronzeBright);
		DrawSquadThumb(Canvas, Page.bPlansThumb, Page.PlansThumb, OriginX, OriginY, Palette.BronzeBright);
	}
}

void ASolidHUD::EnsureSightIndicator()
{
	if (bSightIndicatorAdded)
	{
		return;
	}

	UWorld* World = GetWorld();
	UGameViewportClient* Viewport = World ? World->GetGameViewport() : nullptr;
	if (!Viewport)
	{
		return;
	}

	const TWeakObjectPtr<ASolidHUD> WeakHUD(this);
	const SolidHudTheme::FSightMarkerChip Chip = SolidHudTheme::SightMarkerChip();
	const TSharedRef<SWidget> Widget =
		SNew(SOverlay)
		.Visibility(EVisibility::HitTestInvisible)
		+ SOverlay::Slot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Top)
		.Padding(FMargin(0.f, 16.f, 16.f, 0.f))
		[
			SNew(SBox)
			.Visibility(EVisibility::HitTestInvisible)
			.WidthOverride(Chip.Width)
			.HeightOverride(Chip.Height)
			[
				SNew(SBorder)
				.Visibility(EVisibility::HitTestInvisible)
				.BorderImage(SolidHUDPrivate::SightMarkerBrush())
				.BorderBackgroundColor(FLinearColor::White)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Padding(0.f)
				[
					SNew(STextBlock)
					.Visibility(EVisibility::HitTestInvisible)
					.Font(SolidHUDPrivate::SightMarkerFont())
					.ColorAndOpacity(FLinearColor(0.12f, 0.08f, 0.05f, 1.f))
					.Justification(ETextJustify::Center)
					.Text_Lambda([WeakHUD]()
					{
						ESolidSight Sight = ESolidSight::True;
						if (ASolidHUD* HUD = WeakHUD.Get())
						{
							if (const ASolidCharacter* Captain = Cast<ASolidCharacter>(HUD->GetOwningPawn()))
							{
								Sight = Captain->GetSight();
							}
						}
						return FText::FromString(SolidSight::Marker(Sight));
					})
				]
			]
		];

	Viewport->AddViewportWidgetContent(Widget, 50);
	SolidHUDPrivate::SightIndicators.Add(this, Widget);
	bSightIndicatorAdded = true;
}

void ASolidHUD::RemoveSightIndicator()
{
	TSharedPtr<SWidget> Widget;
	SolidHUDPrivate::SightIndicators.RemoveAndCopyValue(this, Widget);
	bSightIndicatorAdded = false;
	if (!Widget.IsValid())
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		if (UGameViewportClient* Viewport = World->GetGameViewport())
		{
			Viewport->RemoveViewportWidgetContent(Widget.ToSharedRef());
		}
	}
}

void ASolidHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveSightIndicator();
	SolidHUDPrivate::ForgetChoicePointerRows(this);
	Super::EndPlay(EndPlayReason);
}

void ASolidHUD::SetChoicePointerRows(const TArray<SolidMenuPointer::FRow>& Rows)
{
	SolidHUDPrivate::ChoicePointerRows.Add(this, Rows);
}

bool ASolidHUD::SelectChoiceAt(const float X, const float Y)
{
	const TArray<SolidMenuPointer::FRow>* Rows = SolidHUDPrivate::ChoicePointerRows.Find(this);
	if (!Rows)
	{
		return false;
	}

	const int32 Hit = SolidMenuPointer::HitChoice(*Rows, X, Y);
	if (Hit == INDEX_NONE)
	{
		return false;
	}

	if (bShowExitConfirm)
	{
		SetExitConfirmIndex(Hit);
		return true;
	}
	if (bShowPortal)
	{
		SetPortalIndex(Hit);
		return true;
	}
	if (bShowStrongholdChoice)
	{
		const int32 Count = StrongholdChoiceTypes.Num();
		if (Count > 0)
		{
			StrongholdChoiceIndex = FMath::Clamp(Hit, 0, Count - 1);
		}
		return true;
	}
	if (bShowMainMenu)
	{
		SetMainMenuIndex(Hit);
		return true;
	}
	return false;
}

void ASolidHUD::DrawHUD()
{
	Super::DrawHUD();
	if (bShowCharacterEditor)
	{
		if (bSightIndicatorAdded)
		{
			RemoveSightIndicator();
		}
	}
	else
	{
		EnsureSightIndicator();
	}

	if (!Canvas)
	{
		return;
	}

	float BelowTech = 12.f;
	if (bShowDebugPanel && !bShowCharacterEditor)
	{
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
				if (!Companion || !Pawn || Companion->IsStagePreview())
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
		Lines.Add(Captain->IsTrueSight() ? TEXT("WASD  Move") : TEXT("LMB  Move"));
		if (Captain->HasClickMoveDestination())
		{
			Lines.Add(TEXT("Click  moving"));
		}
	}
	Lines.Add(TEXT("F10  Menu"));
	Lines.Add(FString::Printf(TEXT("%s  Inventory"), SolidInventory::ToggleKeyLabel));
	Lines.Add(TEXT("F12  Debug"));
	Lines.Add(TEXT("RMB  Look"));

	if (const ASolidCharacter* Captain = Cast<ASolidCharacter>(Pawn))
	{
		if (Captain->IsSquadFormationDrillActive())
		{
			Lines.Add(FString::Printf(
				TEXT("Drill  leg %d/%d  %s %.2fs"),
				Captain->GetSquadFormationDrillLeg() + 1,
				SolidSquadDrill::NumLegs,
				Captain->IsSquadFormationDrillTurning() ? TEXT("turn") : TEXT("walk"),
				Captain->GetSquadFormationDrillPhaseRemaining()));
		}
	}

	UFont* DebugFont = GEngine ? GEngine->GetSmallFont() : nullptr;
	BelowTech = SolidHUDPrivate::DrawLines(Canvas, DebugFont, Lines, FLinearColor::White);
	}

	UFont* Font = GEngine ? GEngine->GetSmallFont() : nullptr;
	const USolidSquad* Squad = nullptr;
	if (UWorld* World = GetWorld())
	{
		if (const ASolidGameMode* GameMode = World->GetAuthGameMode<ASolidGameMode>())
		{
			Squad = GameMode->GetSquad();
		}
	}
	if (bShowCharacterEditor)
	{
		DrawCharacterEditorPanel();
	}
	else
	{
		SolidHUDPrivate::DrawBattlePlansPanel(Canvas, Font, Squad, BelowTech);
	}

	SolidHUDPrivate::ForgetChoicePointerRows(this);
	if (bShowExitConfirm)
	{
		DrawExitConfirm();
	}
	else if (bShowPortal)
	{
		DrawPortalPopup();
	}
	else if (bShowStrongholdChoice)
	{
		DrawStrongholdChoice();
	}
	else if (bShowSquadBuilder)
	{
		DrawSquadBuilderPopup();
	}
	else if (bShowInventory)
	{
		DrawInventoryPopup();
	}
	else if (bShowJournal)
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

	if (bShowMessage)
	{
		DrawMessagePopup();
	}

	if (APlayerController* PC = GetOwningPlayerController())
	{
		float MouseX = 0.f;
		float MouseY = 0.f;
		bool bHasMouse = PC->GetMousePosition(MouseX, MouseY);
		if (!bHasMouse)
		{
			if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
			{
				FVector2D Position = FVector2D::ZeroVector;
				bHasMouse = Viewport->GetMousePosition(Position);
				MouseX = Position.X;
				MouseY = Position.Y;
			}
		}
		bool bRightDown = PC->IsInputKeyDown(EKeys::RightMouseButton);
		if (const ASolidPlayerController* SolidPC = Cast<ASolidPlayerController>(PC))
		{
			bRightDown = SolidPC->IsRightMouseHeld();
		}
		if (SolidPointer::ShouldDraw(bHasMouse, bRightDown))
		{
			const FVector2D Hotspot(MouseX, MouseY);
			SolidHUDPrivate::DrawPointer(Canvas, Hotspot);
			const bool bShowDragName = (bInventoryDrag && !InventoryDragName.IsEmpty())
				|| (bSquadDrag && !SquadDrag.Label.IsEmpty());
			if (bShowDragName)
			{
				UFont* DragFont = GEngine ? GEngine->GetSmallFont() : nullptr;
				if (DragFont)
				{
					FCanvasTextItem DragItem(
						FVector2D(MouseX + 28.f, MouseY + 18.f),
						FText::FromString(bInventoryDrag ? InventoryDragName : SquadDrag.Label),
						DragFont,
						FLinearColor::White);
					DragItem.EnableShadow(FLinearColor(0.f, 0.f, 0.f, 0.85f));
					Canvas->DrawItem(DragItem);
				}
			}
			if (bShowCharacterEditor && !SkeletonBoneTip.IsEmpty())
			{
				UFont* BoneFont = GEngine ? GEngine->GetMediumFont() : nullptr;
				if (BoneFont)
				{
					float TextW = 0.f;
					float TextH = 0.f;
					Canvas->StrLen(BoneFont, SkeletonBoneTip, TextW, TextH);
					float TipX = MouseX + 28.f;
					float TipY = MouseY + 22.f;
					if (TipX + TextW > static_cast<float>(Canvas->SizeX) - 8.f)
					{
						TipX = MouseX - TextW - 12.f;
					}
					if (TipY + TextH > static_cast<float>(Canvas->SizeY) - 8.f)
					{
						TipY = MouseY - TextH - 12.f;
					}
					FCanvasTextItem BoneItem(
						FVector2D(TipX, TipY),
						FText::FromString(SkeletonBoneTip),
						BoneFont,
						FLinearColor::Black);
					Canvas->DrawItem(BoneItem);
				}
			}
			const FString& PointerTip = CavePassageTip.IsEmpty() ? ForageTip : CavePassageTip;
			if (!PointerTip.IsEmpty())
			{
				UFont* TipFont = GEngine ? GEngine->GetSmallFont() : nullptr;
				float TextW = static_cast<float>(PointerTip.Len() * 8);
				float TextH = 14.f;
				if (TipFont)
				{
					Canvas->StrLen(TipFont, PointerTip, TextW, TextH);
				}
				const float Band = SolidHudTheme::BandThickness(SolidHudTheme::HudFrame());
				const float Pad = 6.f;
				const SolidDungeonPassage::FTipBox Box = SolidDungeonPassage::PlaceTip(
					Hotspot,
					TextW + (Band + Pad) * 2.f,
					TextH + (Band + Pad) * 2.f,
					Canvas->SizeX,
					Canvas->SizeY);
				SolidHUDPrivate::DrawOakFrame(Canvas, Box.X, Box.Y, Box.W, Box.H, SolidHudTheme::HudFrame());
				if (TipFont)
				{
					FCanvasTextItem TextItem(
						FVector2D(Box.X + Band + Pad, Box.Y + Band + Pad),
						FText::FromString(PointerTip),
						TipFont,
						SolidHudTheme::OakAndBronze().Ink);
					TextItem.EnableShadow(FLinearColor(0.f, 0.f, 0.f, 0.85f));
					Canvas->DrawItem(TextItem);
				}
			}
		}
	}
}

void ASolidHUD::SetCavePassageTip(const FString& Tip)
{
	CavePassageTip = Tip;
}

void ASolidHUD::SetForageTip(const FString& Tip)
{
	ForageTip = Tip;
}

void ASolidHUD::ShowMessage(const TCHAR* InMessageId, const TCHAR* Text)
{
	MessageId = InMessageId ? InMessageId : TEXT("");
	MessageText = Text ? Text : TEXT("");
	bShowMessage = !MessageText.IsEmpty();
}

void ASolidHUD::CloseMessage()
{
	if (!bShowMessage)
	{
		return;
	}
	const FString ClosedId = MessageId;
	bShowMessage = false;
	MessageId.Reset();
	MessageText.Reset();
	if (ClosedId.IsEmpty())
	{
		return;
	}
	if (UWorld* World = GetWorld())
	{
		if (ASolidGameMode* GameMode = World->GetAuthGameMode<ASolidGameMode>())
		{
			GameMode->NoteMessageClosed(*ClosedId);
		}
	}
}

void ASolidHUD::DrawMessagePopup() const
{
	TArray<FString> Lines;
	Lines.Add(MessageText);
	Lines.Add(TEXT("Click to close"));
	SolidHUDPrivate::DrawCenteredPopup(Canvas, Lines, 8.f);
}

void ASolidHUD::HandleMenuKey()
{
	if (bShowMessage)
	{
		CloseMessage();
		return;
	}

	if (bShowExitConfirm)
	{
		bShowExitConfirm = false;
		return;
	}

	if (bShowCharacterEditor)
	{
		bShowCharacterEditor = false;
		bCharacterEditorSkeleton = false;
		SkeletonBoneTip.Reset();
		EndCharacterEditorSliderDrag();
		return;
	}

	if (bShowPortal)
	{
		bShowPortal = false;
		return;
	}

	if (bShowStrongholdChoice)
	{
		bShowStrongholdChoice = false;
		StrongholdChoiceTypes.Reset();
		return;
	}

	if (bShowSquadBuilder)
	{
		ForgetSquadBuilderPage();
		return;
	}

	if (bShowJournal)
	{
		bShowJournal = false;
		return;
	}

	if (bShowInventory)
	{
		bShowInventory = false;
		CancelInventoryDrag();
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
	if (bShowMessage)
	{
		CloseMessage();
		return;
	}

	bShowMainMenu = false;
	bShowExitConfirm = false;
	bShowPortal = false;
	bShowStrongholdChoice = false;
	StrongholdChoiceTypes.Reset();
	ForgetSquadBuilderPage();
	bShowCredits = false;
	bShowJournal = false;
	bShowInventory = false;
	CancelInventoryDrag();
	bShowCharacterEditor = false;
	bCharacterEditorSkeleton = false;
	SkeletonBoneTip.Reset();
	EndCharacterEditorSliderDrag();
}

namespace
{
	int32 EditorRosterCount(const ASolidHUD* HUD)
	{
		if (!HUD)
		{
			return 0;
		}
		if (UWorld* World = HUD->GetWorld())
		{
			if (const ASolidGameMode* GameMode = World->GetAuthGameMode<ASolidGameMode>())
			{
				if (const USolidCompany* Company = GameMode->GetCompany())
				{
					return Company->GetCompanionCount();
				}
			}
		}
		return 0;
	}
}

void ASolidHUD::OpenCharacterEditor()
{
	bShowMainMenu = false;
	bShowExitConfirm = false;
	bShowPortal = false;
	ForgetSquadBuilderPage();
	bShowCredits = false;
	bShowJournal = false;
	bShowInventory = false;
	bShowCharacterEditor = true;
	CharacterEditorIndex = 0;
	CharacterEditorSlider = 0;
	bCharacterEditorSkeleton = false;
	SkeletonBoneTip.Reset();
	EndCharacterEditorSliderDrag();
	SolidHUDPrivate::ForgetChoicePointerRows(this);
}

void ASolidHUD::CloseCharacterEditor()
{
	bShowCharacterEditor = false;
	bCharacterEditorSkeleton = false;
	SkeletonBoneTip.Reset();
	EndCharacterEditorSliderDrag();
}

void ASolidHUD::SetCharacterEditorSkeleton(const bool bShow)
{
	bCharacterEditorSkeleton = bShow;
	if (!bShow)
	{
		SkeletonBoneTip.Reset();
	}
}

void ASolidHUD::SetSkeletonBoneTip(const FString& Tip)
{
	SkeletonBoneTip = Tip;
}

void ASolidHUD::MoveCharacterEditor(const int32 Delta)
{
	CharacterEditorIndex = SolidCharacterEditor::WrapIndex(
		CharacterEditorIndex,
		Delta,
		EditorRosterCount(this));
}

void ASolidHUD::MoveCharacterEditorSlider(const int32 Delta)
{
	CharacterEditorSlider = SolidBodyControls::WrapSlider(CharacterEditorSlider, Delta);
}

bool ASolidHUD::PressCharacterEditorSlider(
	const float X,
	const float Y,
	int32& OutSlider,
	bool& bBar,
	float& OutFraction)
{
	bBar = false;
	OutFraction = 0.f;
	const FSolidSliderHit Hit = SolidCharacterEditor::HitSliders(
		CharacterEditorSliderRows.GetData(),
		CharacterEditorSliderRows.Num(),
		X,
		Y);
	if (Hit.Kind == ESolidSliderHit::None)
	{
		return false;
	}

	CharacterEditorSlider = Hit.Slider;
	OutSlider = Hit.Slider;
	bCharacterEditorSliderDrag = Hit.Kind == ESolidSliderHit::Bar;
	CharacterEditorSliderDragIndex = bCharacterEditorSliderDrag ? Hit.Slider : INDEX_NONE;
	bBar = bCharacterEditorSliderDrag;
	if (bBar)
	{
		for (const FSolidSliderRow& Row : CharacterEditorSliderRows)
		{
			if (Row.Slider == Hit.Slider)
			{
				OutFraction = SolidCharacterEditor::BarFraction(Row.Bar, X);
				break;
			}
		}
	}
	return true;
}

bool ASolidHUD::DragCharacterEditorSlider(const float X, int32& OutSlider, float& OutFraction)
{
	if (!bCharacterEditorSliderDrag)
	{
		return false;
	}
	for (const FSolidSliderRow& Row : CharacterEditorSliderRows)
	{
		if (Row.Slider != CharacterEditorSliderDragIndex)
		{
			continue;
		}
		CharacterEditorSlider = Row.Slider;
		OutSlider = Row.Slider;
		OutFraction = SolidCharacterEditor::BarFraction(Row.Bar, X);
		return true;
	}
	return false;
}

void ASolidHUD::EndCharacterEditorSliderDrag()
{
	bCharacterEditorSliderDrag = false;
	CharacterEditorSliderDragIndex = INDEX_NONE;
}

void ASolidHUD::DrawCharacterEditorPanel()
{
	CharacterEditorSliderRows.Reset();

	TArray<FString> Lines;
	const USolidCompany* Company = nullptr;
	if (UWorld* World = GetWorld())
	{
		if (const ASolidGameMode* GameMode = World->GetAuthGameMode<ASolidGameMode>())
		{
			Company = GameMode->GetCompany();
		}
	}

	const FSolidCompanion* Companion = Company
		? Company->GetCompanion(CharacterEditorIndex)
		: nullptr;
	if (Companion)
	{
		SolidCharacterEditor::Describe(*Companion, Lines);
	}
	else
	{
		Lines.Add(TEXT("Character editor"));
		Lines.Add(TEXT("No companions"));
		Lines.Add(TEXT("F10  open world"));
	}
	Lines.Add(SolidCharacterEditor::SkeletonHint(bCharacterEditorSkeleton));
	Lines.Add(SolidCharacterEditor::TurnClockwiseHint());
	Lines.Add(SolidCharacterEditor::TurnAnticlockwiseHint());

	if (!Canvas || Lines.Num() == 0)
	{
		return;
	}

	UFont* Font = GEngine ? GEngine->GetSmallFont() : nullptr;
	const float PadX = 16.f;
	const float StartY = 12.f;
	const float LineGap = 2.f;
	const float BoxPad = 6.f;

	float MaxWidth = 0.f;
	float LineHeight = 14.f;
	bool bHasSlider = false;
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
		if (SolidCharacterEditor::SliderForLine(Line) != INDEX_NONE)
		{
			bHasSlider = true;
		}
	}

	const float SliderExtra = bHasSlider
		? SolidCharacterEditor::SliderBarGap + SolidCharacterEditor::SliderBarWidth
		: 0.f;
	const float BlockHeight = Lines.Num() * LineHeight + (Lines.Num() - 1) * LineGap;
	FCanvasTileItem Background(
		FVector2D(PadX - BoxPad, StartY - BoxPad * 0.5f),
		FVector2D(MaxWidth + SliderExtra + BoxPad * 2.f, BlockHeight + BoxPad),
		FLinearColor(0.f, 0.f, 0.f, 0.55f));
	Background.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Background);

	const FLinearColor Amber(1.f, 0.78f, 0.28f);
	const float TextRight = PadX + MaxWidth;
	float Y = StartY;
	for (const FString& Line : Lines)
	{
		const int32 Slider = SolidCharacterEditor::SliderForLine(Line);
		const bool bSelected = Slider != INDEX_NONE && Slider == CharacterEditorSlider;
		if (Slider != INDEX_NONE)
		{
			const FSolidSliderRow Row = SolidCharacterEditor::MakeSliderRow(
				Slider, PadX, Y, LineHeight, TextRight);
			CharacterEditorSliderRows.Add(Row);

			float Value = 0.f;
			if (Companion)
			{
				if (Slider == SolidBodyControls::Build)
				{
					Value = Companion->BodyBuild;
				}
				else if (Slider == SolidBodyControls::Gender)
				{
					Value = Companion->BodyGender;
				}
				else
				{
					Value = Companion->BodyHeight;
				}
			}
			const float Fraction = SolidBodyControls::Fraction(Slider, Value);
			const float Inset = 2.f;
			const float TrackY = Row.Bar.Y + Inset;
			const float TrackH = FMath::Max(4.f, Row.Bar.H - Inset * 2.f);
			SolidHUDPrivate::FillRect(
				Canvas,
				Row.Bar.X - 1.f,
				TrackY - 1.f,
				Row.Bar.W + 2.f,
				TrackH + 2.f,
				FLinearColor(0.85f, 0.85f, 0.85f));
			SolidHUDPrivate::FillRect(
				Canvas,
				Row.Bar.X,
				TrackY,
				Row.Bar.W,
				TrackH,
				FLinearColor(0.16f, 0.16f, 0.16f));
			SolidHUDPrivate::FillRect(
				Canvas,
				Row.Bar.X,
				TrackY,
				FMath::Max(0.f, Row.Bar.W * Fraction),
				TrackH,
				bSelected ? Amber : FLinearColor(0.72f, 0.72f, 0.74f));
		}

		FCanvasTextItem TextItem(
			FVector2D(PadX, Y),
			FText::FromString(Line),
			Font,
			bSelected ? Amber : FLinearColor::White);
		TextItem.EnableShadow(FLinearColor(0.f, 0.f, 0.f, 0.9f));
		Canvas->DrawItem(TextItem);
		Y += LineHeight + LineGap;
	}
}

void ASolidHUD::CancelSquadDrag()
{
	bSquadDrag = false;
	SquadDrag = SolidSquadBuilder::FDrag();
}

void ASolidHUD::ForgetSquadBuilderPage()
{
	bShowSquadBuilder = false;
	bSquadPageReady = false;
	SquadBuilderView = SolidSquadBuilder::FView();
	SquadPage = SolidSquadBuilder::FPage();
	CancelSquadDrag();
}

void ASolidHUD::OpenSquadBuilder()
{
	bShowMainMenu = false;
	bShowExitConfirm = false;
	bShowPortal = false;
	bShowCredits = false;
	bShowJournal = false;
	bShowInventory = false;
	CancelInventoryDrag();
	bShowCharacterEditor = false;
	bCharacterEditorSkeleton = false;
	SkeletonBoneTip.Reset();
	ForgetSquadBuilderPage();
	bShowSquadBuilder = true;
	SolidHUDPrivate::ForgetChoicePointerRows(this);
}

void ASolidHUD::PressSquadBuilder(const float X, const float Y)
{
	if (!bSquadPageReady)
	{
		return;
	}
	const SolidSquadBuilder::FHit Hit = SolidSquadBuilder::HitTest(
		SquadPage,
		X - SquadOriginX,
		Y - SquadOriginY);
	if (Hit.Kind == SolidSquadBuilder::EHit::VocationFilter
		&& Hit.Index >= 0
		&& Hit.Index < SolidSquadBuilder::VocationFilterCount)
	{
		SolidSquadBuilder::ChooseVocation(
			SquadBuilderView,
			static_cast<SolidSquadBuilder::EVocationFilter>(Hit.Index));
		return;
	}
	if (Hit.Kind == SolidSquadBuilder::EHit::FormationFilter
		&& Hit.Index >= 0
		&& Hit.Index < SolidSquadBuilder::FormationFilterCount)
	{
		SolidSquadBuilder::ChooseFormation(
			SquadBuilderView,
			static_cast<SolidSquadBuilder::EFormationFilter>(Hit.Index));
		return;
	}
	SolidSquadBuilder::FDrag Drag;
	if (!SolidSquadBuilder::BeginDrag(SquadPage, Hit, Drag))
	{
		return;
	}
	bSquadDrag = true;
	SquadDrag = Drag;
}

void ASolidHUD::ReleaseSquadBuilder(const float X, const float Y)
{
	if (!bSquadDrag)
	{
		return;
	}
	const SolidSquadBuilder::FDrag Drag = SquadDrag;
	CancelSquadDrag();
	if (!bSquadPageReady)
	{
		return;
	}
	const SolidSquadBuilder::FHit Hit = SolidSquadBuilder::HitTest(
		SquadPage,
		X - SquadOriginX,
		Y - SquadOriginY);
	UWorld* World = GetWorld();
	ASolidGameMode* GameMode = World ? World->GetAuthGameMode<ASolidGameMode>() : nullptr;
	if (!GameMode)
	{
		return;
	}
	if (SolidSquadBuilder::Drop(GameMode->GetSquad(), GameMode->GetMaxSquadSize(), Drag, Hit)
		&& Drag.Kind == SolidSquadBuilder::EDrag::Companion)
	{
		GameMode->RefreshFieldedSquad();
	}
}

void ASolidHUD::ScrollSquadBuilder(const float X, const float Y, const float Wheel)
{
	if (!bSquadPageReady)
	{
		return;
	}
	SolidSquadBuilder::Scroll(SquadPage, X - SquadOriginX, Y - SquadOriginY, Wheel, SquadBuilderView);
}

void ASolidHUD::OpenPortal()
{
	bShowMainMenu = false;
	bShowExitConfirm = false;
	bShowCredits = false;
	bShowJournal = false;
	bShowInventory = false;
	ForgetSquadBuilderPage();
	bShowCharacterEditor = false;
	bCharacterEditorSkeleton = false;
	SkeletonBoneTip.Reset();
	bShowPortal = true;
	PortalIndex = 0;
	SolidHUDPrivate::ForgetChoicePointerRows(this);
}

void ASolidHUD::ClosePortal()
{
	bShowPortal = false;
}

void ASolidHUD::MovePortalSelection(const int32 Delta)
{
	PortalIndex = SolidPortal::WrapIndex(PortalIndex, Delta);
}

void ASolidHUD::SetPortalIndex(const int32 Index)
{
	const int32 Count = SolidPortal::TownCount();
	if (Count <= 0)
	{
		PortalIndex = 0;
		return;
	}
	PortalIndex = FMath::Clamp(Index, 0, Count - 1);
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

void ASolidHUD::ToggleDebugPanel()
{
	bShowDebugPanel = !bShowDebugPanel;
}

void ASolidHUD::DrawCreditsPopup() const
{
	TArray<FString> Lines;
	SolidCredits::CollectLines(Lines);
	SolidHUDPrivate::DrawCenteredPopup(Canvas, Lines, 4.f);
}

void ASolidHUD::ToggleInventory()
{
	if (bShowMessage)
	{
		return;
	}
	if (bShowInventory)
	{
		bShowInventory = false;
		CancelInventoryDrag();
		return;
	}

	bShowMainMenu = false;
	bShowExitConfirm = false;
	bShowPortal = false;
	ForgetSquadBuilderPage();
	bShowCredits = false;
	bShowJournal = false;
	bShowCharacterEditor = false;
	bCharacterEditorSkeleton = false;
	SkeletonBoneTip.Reset();
	EndCharacterEditorSliderDrag();
	SolidHUDPrivate::ForgetChoicePointerRows(this);
	bShowInventory = true;
}

void ASolidHUD::CancelInventoryDrag()
{
	bInventoryDrag = false;
	bCraftButtonPressed = false;
	InventoryDragFrom = SolidInventory::FSlotHit();
	InventoryDragName.Reset();
}

void ASolidHUD::PressInventory(const float X, const float Y)
{
	bCraftButtonPressed = false;
	if (!bInventoryHitReady)
	{
		return;
	}
	const float LocalX = X - InventoryOriginX;
	const float LocalY = Y - InventoryOriginY;
	if (SolidInventory::HitCraftButton(InventoryPage, LocalX, LocalY))
	{
		bCraftButtonPressed = InventoryPage.bCraftEnabled;
		return;
	}
	SolidInventory::FSlotHit Hit;
	if (!SolidInventory::HitSlot(InventoryPage, LocalX, LocalY, Hit) || !Hit.bHit)
	{
		return;
	}
	UWorld* World = GetWorld();
	ASolidGameMode* GameMode = World ? World->GetAuthGameMode<ASolidGameMode>() : nullptr;
	if (!GameMode)
	{
		return;
	}
	const FGameState& State = GameMode->GetConfiguration();
	const SolidStorage::FContainer* Chest = bInventoryChestOpen ? &State.CaveChest : nullptr;
	const TCHAR* ItemId = SolidInventory::ItemAt(State.Inventory, Chest, Hit);
	if (!ItemId || !ItemId[0])
	{
		return;
	}
	bInventoryDrag = true;
	InventoryDragFrom = Hit;
	InventoryDragName = SolidItemType::PageLabel(
		ItemId,
		SolidInventory::CountAt(State.Inventory, Chest, Hit));
}

void ASolidHUD::ReleaseInventory(const float X, const float Y)
{
	if (bCraftButtonPressed)
	{
		bCraftButtonPressed = false;
		if (!bInventoryHitReady || !InventoryPage.bCraftEnabled)
		{
			return;
		}
		if (!SolidInventory::HitCraftButton(InventoryPage, X - InventoryOriginX, Y - InventoryOriginY))
		{
			return;
		}
		UWorld* World = GetWorld();
		ASolidGameMode* GameMode = World ? World->GetAuthGameMode<ASolidGameMode>() : nullptr;
		if (GameMode)
		{
			GameMode->CraftFromInventory();
		}
		return;
	}
	if (!bInventoryDrag)
	{
		return;
	}
	const SolidInventory::FSlotHit From = InventoryDragFrom;
	const bool bChestOpen = bInventoryChestOpen;
	CancelInventoryDrag();
	if (!bInventoryHitReady)
	{
		return;
	}
	SolidInventory::FSlotHit Hit;
	if (!SolidInventory::HitSlot(InventoryPage, X - InventoryOriginX, Y - InventoryOriginY, Hit) || !Hit.bHit)
	{
		return;
	}
	UWorld* World = GetWorld();
	ASolidGameMode* GameMode = World ? World->GetAuthGameMode<ASolidGameMode>() : nullptr;
	if (!GameMode)
	{
		return;
	}
	GameMode->MoveCarriedItem(From, Hit, bChestOpen);
}

void ASolidHUD::DrawInventoryPopup()
{
	bInventoryHitReady = false;
	bInventoryChestOpen = false;
	FGameState State;
	const ASolidGameMode* GameMode = nullptr;
	if (UWorld* World = GetWorld())
	{
		GameMode = World->GetAuthGameMode<ASolidGameMode>();
		if (GameMode)
		{
			State = GameMode->GetConfiguration();
		}
	}

	FIntPoint TowerCell = FIntPoint::ZeroValue;
	if (const USolidGameInstance* Instance = GameMode ? GameMode->GetGameInstance<USolidGameInstance>() : nullptr)
	{
		TowerCell = Instance->GetSession().TowerCell;
	}
	const bool bShowChest = GameMode
		&& SolidStrongholdTransition::CompanyChestOpen(
			State,
			GameMode->IsInDungeon(),
			GameMode->IsInTower(),
			TowerCell);
	SolidInventory::FPage Page;
	SolidInventory::Layout(State.Inventory, Page, bShowChest ? &State.CaveChest : nullptr);

	float LineHeight = 16.f;
	if (Canvas)
	{
		if (UFont* Font = GEngine ? GEngine->GetSmallFont() : nullptr)
		{
			float HintW = 0.f;
			Canvas->StrLen(Font, SolidInventory::CloseHint, HintW, LineHeight);
		}
		const SolidInventory::FView View = SolidInventory::ContentOrigin(
			static_cast<float>(Canvas->SizeX),
			static_cast<float>(Canvas->SizeY),
			Page,
			LineHeight);
		InventoryPage = Page;
		InventoryOriginX = View.OriginX;
		InventoryOriginY = View.OriginY;
		bInventoryChestOpen = bShowChest;
		bInventoryHitReady = true;
	}

	SolidHUDPrivate::DrawInventoryBoard(Canvas, Page);
}

void ASolidHUD::DrawJournalPopup() const
{
	FGameState State;
	if (UWorld* World = GetWorld())
	{
		if (const ASolidGameMode* GameMode = World->GetAuthGameMode<ASolidGameMode>())
		{
			State = GameMode->GetConfiguration();
		}
	}

	TArray<SolidJournal::FRow> Rows;
	SolidJournal::CollectRows(State, Rows);
	SolidHUDPrivate::DrawJournalBoard(Canvas, Rows);
}

void ASolidHUD::DrawMainMenuPopup()
{
	int32 MaxSquadSize = 0;
	if (UWorld* World = GetWorld())
	{
		if (const ASolidGameMode* GameMode = World->GetAuthGameMode<ASolidGameMode>())
		{
			MaxSquadSize = GameMode->GetMaxSquadSize();
		}
	}

	TArray<FSolidMainMenuEntry> Entries;
	SolidMainMenu::CollectEntries(Entries);
	TArray<FString> Labels;
	TArray<bool> Enabled;
	for (const FSolidMainMenuEntry& Entry : Entries)
	{
		Labels.Add(Entry.Label);
		const bool bSquad = Entry.Item != ESolidMainMenuItem::BuildSquad
			|| SolidSquadBuilder::CanOpen(MaxSquadSize);
		Enabled.Add(bSquad);
	}
	TArray<SolidMenuPointer::FRow> Rows;
	SolidHUDPrivate::DrawChoicePopup(Canvas, TEXT("Main Menu"), Labels, MainMenuIndex, Rows, &Enabled);
	SetChoicePointerRows(Rows);
}

void ASolidHUD::OpenExitConfirm()
{
	bShowMainMenu = false;
	bShowCredits = false;
	bShowJournal = false;
	bShowInventory = false;
	CancelInventoryDrag();
	ForgetSquadBuilderPage();
	bShowCharacterEditor = false;
	bCharacterEditorSkeleton = false;
	SkeletonBoneTip.Reset();
	bShowPortal = false;
	bShowStrongholdChoice = false;
	StrongholdChoiceTypes.Reset();
	bShowExitConfirm = true;
	ExitConfirmIndex = SolidExit::YesIndex;
	SolidHUDPrivate::ForgetChoicePointerRows(this);
}

void ASolidHUD::CloseExitConfirm()
{
	bShowExitConfirm = false;
}

void ASolidHUD::MoveExitConfirm(const int32 Delta)
{
	ExitConfirmIndex = SolidExit::WrapIndex(ExitConfirmIndex, Delta);
}

void ASolidHUD::SetExitConfirmIndex(const int32 Index)
{
	ExitConfirmIndex = FMath::Clamp(Index, 0, SolidExit::ChoiceCount - 1);
}

bool ASolidHUD::AcceptExitConfirm()
{
	if (!bShowExitConfirm)
	{
		return false;
	}
	const bool bYes = SolidExit::IsYes(ExitConfirmIndex);
	bShowExitConfirm = false;
	return bYes;
}

void ASolidHUD::DrawExitConfirm()
{
	TArray<FString> Labels;
	Labels.Add(SolidExit::Label(SolidExit::YesIndex));
	Labels.Add(SolidExit::Label(SolidExit::NoIndex));
	TArray<SolidMenuPointer::FRow> Rows;
	SolidHUDPrivate::DrawChoicePopup(Canvas, SolidExit::Prompt(), Labels, ExitConfirmIndex, Rows);
	SetChoicePointerRows(Rows);
}

void ASolidHUD::DrawSquadBuilderPopup()
{
	bSquadPageReady = false;
	const USolidSquad* Squad = nullptr;
	int32 MaxSquadSize = 0;
	if (UWorld* World = GetWorld())
	{
		if (const ASolidGameMode* GameMode = World->GetAuthGameMode<ASolidGameMode>())
		{
			Squad = GameMode->GetSquad();
			MaxSquadSize = GameMode->GetMaxSquadSize();
		}
	}

	SolidSquadBuilder::FPage Page;
	SolidSquadBuilder::Layout(Squad, MaxSquadSize, SquadBuilderView, Page);
	SquadBuilderView.CompanyScroll = Page.CompanyScroll;
	SquadBuilderView.PlansScroll = Page.PlansScroll;
	SquadBuilderView.SquadScroll = Page.SquadScroll;

	if (!Canvas)
	{
		return;
	}

	UFont* TitleFont = GEngine ? GEngine->GetMediumFont() : nullptr;
	UFont* Font = GEngine ? GEngine->GetSmallFont() : nullptr;
	if (!TitleFont)
	{
		TitleFont = Font;
	}
	float TitleW = static_cast<float>(Page.Title.Len() * 8);
	float TitleH = 18.f;
	if (TitleFont)
	{
		Canvas->StrLen(TitleFont, Page.Title, TitleW, TitleH);
	}

	float ContentW = 0.f;
	float ContentH = 0.f;
	SolidSquadBuilder::ContentSize(ContentW, ContentH);
	float BoardW = 0.f;
	float BoardH = 0.f;
	SolidJournal::BoardSize(
		static_cast<float>(Canvas->SizeX),
		static_cast<float>(Canvas->SizeY),
		ContentW,
		ContentH,
		BoardW,
		BoardH);
	const float BoxX = (static_cast<float>(Canvas->SizeX) - BoardW) * 0.5f;
	const float BoxY = (static_cast<float>(Canvas->SizeY) - BoardH) * 0.5f;
	const float OriginX = BoxX + SolidJournal::BoardPad;
	const float OriginY = BoxY + SolidJournal::BoardPad;

	FCanvasTileItem Dim(
		FVector2D(0.f, 0.f),
		FVector2D(static_cast<float>(Canvas->SizeX), static_cast<float>(Canvas->SizeY)),
		FLinearColor(0.f, 0.f, 0.f, 0.72f));
	Dim.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Dim);

	const SolidHudTheme::FFrame Frame = SolidHudTheme::MenuFrame();
	SolidHUDPrivate::DrawOakFrame(Canvas, BoxX, BoxY, BoardW, BoardH, Frame);
	const SolidHudTheme::FPalette Palette = SolidHudTheme::OakAndBronze();
	const float Band = SolidHudTheme::BandThickness(Frame);
	const SolidJournal::FTitlePlate Plate = SolidJournal::TitlePlate(BoxX, BoxY, BoardW, TitleW, TitleH, Band);
	SolidHUDPrivate::FillRect(Canvas, Plate.X, Plate.Y, Plate.W, Plate.H, Palette.Bronze);
	if (TitleFont)
	{
		const float NameX = Plate.X + (Plate.W - TitleW) * 0.5f;
		const float NameY = Plate.Y + (Plate.H - TitleH) * 0.5f;
		FCanvasTextItem NameItem(FVector2D(NameX, NameY), FText::FromString(Page.Title), TitleFont, Palette.Title);
		NameItem.EnableShadow(FLinearColor(0.f, 0.f, 0.f, 0.85f));
		Canvas->DrawItem(NameItem);
	}

	SolidHUDPrivate::DrawSquadPage(Canvas, Page, OriginX, OriginY);
	SquadPage = Page;
	SquadOriginX = OriginX;
	SquadOriginY = OriginY;
	bSquadPageReady = true;
}

void ASolidHUD::OpenStrongholdChoice(
	const FIntPoint Cell,
	const ESolidStrongholdSize Size,
	const TArray<ESolidStrongholdType>& Types)
{
	if (Types.Num() == 0)
	{
		return;
	}

	bShowMainMenu = false;
	bShowExitConfirm = false;
	bShowCredits = false;
	bShowJournal = false;
	bShowInventory = false;
	ForgetSquadBuilderPage();
	bShowCharacterEditor = false;
	bCharacterEditorSkeleton = false;
	SkeletonBoneTip.Reset();
	bShowPortal = false;
	bShowStrongholdChoice = true;
	StrongholdChoiceIndex = 0;
	StrongholdChoiceCell = Cell;
	StrongholdChoiceSize = static_cast<uint8>(Size);
	StrongholdChoiceTypes.Reset();
	for (const ESolidStrongholdType Type : Types)
	{
		StrongholdChoiceTypes.Add(static_cast<uint8>(Type));
	}
	SolidHUDPrivate::ForgetChoicePointerRows(this);
}

void ASolidHUD::CloseStrongholdChoice()
{
	bShowStrongholdChoice = false;
	StrongholdChoiceTypes.Reset();
}

void ASolidHUD::SetStrongholdChoiceIndex(const int32 Index)
{
	if (StrongholdChoiceTypes.IsValidIndex(Index))
	{
		StrongholdChoiceIndex = Index;
	}
}

void ASolidHUD::MoveStrongholdChoice(const int32 Delta)
{
	const int32 Count = StrongholdChoiceTypes.Num();
	if (Count <= 0)
	{
		StrongholdChoiceIndex = 0;
		return;
	}
	int32 Next = StrongholdChoiceIndex + Delta;
	Next %= Count;
	if (Next < 0)
	{
		Next += Count;
	}
	StrongholdChoiceIndex = Next;
}

ESolidStrongholdType ASolidHUD::GetStrongholdChoiceType() const
{
	if (!StrongholdChoiceTypes.IsValidIndex(StrongholdChoiceIndex))
	{
		return ESolidStrongholdType::Tower;
	}
	return static_cast<ESolidStrongholdType>(StrongholdChoiceTypes[StrongholdChoiceIndex]);
}

void ASolidHUD::DrawStrongholdChoice()
{
	TArray<FString> Labels;
	for (const uint8 Type : StrongholdChoiceTypes)
	{
		Labels.Add(SolidStronghold::TypeName(static_cast<ESolidStrongholdType>(Type)));
	}
	TArray<SolidMenuPointer::FRow> Rows;
	SolidHUDPrivate::DrawChoicePopup(Canvas, TEXT("Choose a type"), Labels, StrongholdChoiceIndex, Rows);
	SetChoicePointerRows(Rows);
}

void ASolidHUD::DrawPortalPopup()
{
	TArray<SolidPortal::FChoice> Choices;
	SolidPortal::CollectChoices(Choices);
	TArray<FString> Labels;
	for (const SolidPortal::FChoice& Choice : Choices)
	{
		Labels.Add(Choice.Name);
	}
	TArray<SolidMenuPointer::FRow> Rows;
	SolidHUDPrivate::DrawChoicePopup(Canvas, TEXT("Portal"), Labels, PortalIndex, Rows);
	SetChoicePointerRows(Rows);
}
