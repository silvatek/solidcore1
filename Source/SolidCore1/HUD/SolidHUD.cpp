#include "SolidHUD.h"
#include "HUD/SolidHudTheme.h"
#include "HUD/SolidPointer.h"
#include "Strongholds/SolidDungeonPassage.h"
#include "SolidBuildId.h"
#include "Menus/SolidCharacterEditor.h"
#include "SolidBodyParams.h"
#include "Menus/SolidCredits.h"
#include "Inventory/SolidInventory.h"
#include "Journal/SolidJournal.h"
#include "Menus/SolidMainMenu.h"
#include "Menus/SolidMenuPointer.h"
#include "Menus/SolidPartyBuilder.h"
#include "Menus/SolidPortal.h"
#include "SolidCharacter.h"
#include "SolidSight.h"
#include "SolidGameMode.h"
#include "SolidPlayerController.h"
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
		const USolidParty* Party,
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

		const float OriginX = BoxX + (BoardW - Page.ContentW) * 0.5f;
		const float OriginY = BoxY + SolidJournal::BoardPad;
		if (Font && !FString(SolidInventory::PackTitle).IsEmpty())
		{
			float PackW = 0.f;
			float PackH = 0.f;
			Canvas->StrLen(Font, SolidInventory::PackTitle, PackW, PackH);
			FCanvasTextItem PackItem(
				FVector2D(OriginX + Page.PackX + (Page.PackGridW - PackW) * 0.5f, OriginY + Page.PackLabelY),
				FText::FromString(SolidInventory::PackTitle),
				Font,
				Palette.Title);
			PackItem.EnableShadow(FLinearColor(0.f, 0.f, 0.f, 0.85f));
			Canvas->DrawItem(PackItem);
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
			if (Slot.Item.IsEmpty())
			{
				DrawCenteredLine(Canvas, Font, Slot.Label, X, Y, Slot.W, Slot.H, Palette.Hint);
				continue;
			}
			const float LabelBand = 18.f;
			DrawCenteredLine(Canvas, Font, Slot.Label, X, Y, Slot.W, LabelBand, Palette.Hint);
			DrawCenteredLine(
				Canvas,
				Font,
				Slot.Item,
				X,
				Y + LabelBand,
				Slot.W,
				FMath::Max(0.f, Slot.H - LabelBand),
				Palette.Ink);
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

	if (bShowPortal)
	{
		SetPortalIndex(Hit);
		return true;
	}
	if (bShowPartyBuilder)
	{
		SetPartyBuilderIndex(Hit);
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

	UFont* DebugFont = GEngine ? GEngine->GetSmallFont() : nullptr;
	BelowTech = SolidHUDPrivate::DrawLines(Canvas, DebugFont, Lines, FLinearColor::White);
	}

	UFont* Font = GEngine ? GEngine->GetSmallFont() : nullptr;
	const USolidParty* Party = nullptr;
	if (UWorld* World = GetWorld())
	{
		if (const ASolidGameMode* GameMode = World->GetAuthGameMode<ASolidGameMode>())
		{
			Party = GameMode->GetParty();
		}
	}
	if (bShowCharacterEditor)
	{
		DrawCharacterEditorPanel();
	}
	else
	{
		SolidHUDPrivate::DrawBattlePlansPanel(Canvas, Font, Party, BelowTech);
	}

	SolidHUDPrivate::ForgetChoicePointerRows(this);
	if (bShowPortal)
	{
		DrawPortalPopup();
	}
	else if (bShowPartyBuilder)
	{
		DrawPartyBuilderPopup();
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
			if (!CavePassageTip.IsEmpty())
			{
				UFont* TipFont = GEngine ? GEngine->GetSmallFont() : nullptr;
				float TextW = static_cast<float>(CavePassageTip.Len() * 8);
				float TextH = 14.f;
				if (TipFont)
				{
					Canvas->StrLen(TipFont, CavePassageTip, TextW, TextH);
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
						FText::FromString(CavePassageTip),
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

	if (bShowPartyBuilder)
	{
		bShowPartyBuilder = false;
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
	bShowPortal = false;
	bShowPartyBuilder = false;
	bShowCredits = false;
	bShowJournal = false;
	bShowInventory = false;
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
	bShowPortal = false;
	bShowPartyBuilder = false;
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

void ASolidHUD::OpenPartyBuilder()
{
	bShowMainMenu = false;
	bShowPortal = false;
	bShowCredits = false;
	bShowJournal = false;
	bShowInventory = false;
	bShowCharacterEditor = false;
	bCharacterEditorSkeleton = false;
	SkeletonBoneTip.Reset();
	bShowPartyBuilder = true;
	PartyBuilderIndex = 0;
	SolidHUDPrivate::ForgetChoicePointerRows(this);
}

void ASolidHUD::MovePartyBuilderSelection(const int32 Delta)
{
	int32 Count = 0;
	if (UWorld* World = GetWorld())
	{
		if (const ASolidGameMode* GameMode = World->GetAuthGameMode<ASolidGameMode>())
		{
			if (const USolidParty* Party = GameMode->GetParty())
			{
				Count = Party->GetCompany() ? Party->GetCompany()->GetCompanionCount() : 0;
			}
		}
	}
	if (Count <= 0)
	{
		PartyBuilderIndex = 0;
		return;
	}

	int32 Wrapped = (PartyBuilderIndex + Delta) % Count;
	if (Wrapped < 0)
	{
		Wrapped += Count;
	}
	PartyBuilderIndex = Wrapped;
}

void ASolidHUD::SetPartyBuilderIndex(const int32 Index)
{
	int32 Count = 0;
	if (UWorld* World = GetWorld())
	{
		if (const ASolidGameMode* GameMode = World->GetAuthGameMode<ASolidGameMode>())
		{
			if (const USolidParty* Party = GameMode->GetParty())
			{
				Count = Party->GetCompany() ? Party->GetCompany()->GetCompanionCount() : 0;
			}
		}
	}
	if (Count <= 0)
	{
		PartyBuilderIndex = 0;
		return;
	}
	PartyBuilderIndex = FMath::Clamp(Index, 0, Count - 1);
}

void ASolidHUD::OpenPortal()
{
	bShowMainMenu = false;
	bShowCredits = false;
	bShowJournal = false;
	bShowInventory = false;
	bShowPartyBuilder = false;
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
		return;
	}

	bShowMainMenu = false;
	bShowPortal = false;
	bShowPartyBuilder = false;
	bShowCredits = false;
	bShowJournal = false;
	bShowCharacterEditor = false;
	bCharacterEditorSkeleton = false;
	SkeletonBoneTip.Reset();
	EndCharacterEditorSliderDrag();
	SolidHUDPrivate::ForgetChoicePointerRows(this);
	bShowInventory = true;
}

void ASolidHUD::DrawInventoryPopup() const
{
	FGameState State;
	if (UWorld* World = GetWorld())
	{
		if (const ASolidGameMode* GameMode = World->GetAuthGameMode<ASolidGameMode>())
		{
			State = GameMode->GetConfiguration();
		}
	}

	SolidInventory::FPage Page;
	SolidInventory::Layout(State.Inventory, Page);
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
	int32 MaxPartySize = 0;
	bool bInDungeon = false;
	bool bInLair = false;
	bool bCaveEnabled = false;
	if (UWorld* World = GetWorld())
	{
		if (const ASolidGameMode* GameMode = World->GetAuthGameMode<ASolidGameMode>())
		{
			MaxPartySize = GameMode->GetMaxPartySize();
			bInDungeon = GameMode->IsInDungeon();
			bInLair = GameMode->IsInLair();
			bCaveEnabled = GameMode->IsCaveEnabled();
		}
	}

	TArray<FSolidMainMenuEntry> Entries;
	SolidMainMenu::CollectEntries(Entries, bInDungeon);
	TArray<FString> Labels;
	TArray<bool> Enabled;
	for (const FSolidMainMenuEntry& Entry : Entries)
	{
		Labels.Add(Entry.Label);
		const bool bParty = Entry.Item != ESolidMainMenuItem::BuildParty
			|| SolidPartyBuilder::CanOpen(MaxPartySize);
		const bool bCave = Entry.Item != ESolidMainMenuItem::Dungeon
			|| (!bInLair && SolidMainMenu::CanUseCaveRow(bCaveEnabled, bInDungeon));
		Enabled.Add(bParty && bCave);
	}
	TArray<SolidMenuPointer::FRow> Rows;
	SolidHUDPrivate::DrawChoicePopup(Canvas, TEXT("Main Menu"), Labels, MainMenuIndex, Rows, &Enabled);
	SetChoicePointerRows(Rows);
}

void ASolidHUD::DrawPartyBuilderPopup()
{
	const USolidParty* Party = nullptr;
	int32 MaxPartySize = 0;
	if (UWorld* World = GetWorld())
	{
		if (const ASolidGameMode* GameMode = World->GetAuthGameMode<ASolidGameMode>())
		{
			Party = GameMode->GetParty();
			MaxPartySize = GameMode->GetMaxPartySize();
		}
	}

	TArray<SolidPartyBuilder::FRow> Choices;
	SolidPartyBuilder::CollectRows(Party, Choices);
	int32 SelectedCount = 0;
	for (const SolidPartyBuilder::FRow& Choice : Choices)
	{
		if (Choice.bSelected)
		{
			++SelectedCount;
		}
	}

	TArray<FString> Labels;
	TArray<bool> Enabled;
	for (const SolidPartyBuilder::FRow& Choice : Choices)
	{
		Labels.Add(Choice.Label);
		Enabled.Add(SolidPartyBuilder::IsChoiceAvailable(Choice.bSelected, SelectedCount, MaxPartySize));
	}

	const FString Title = FString::Printf(TEXT("Build party  %d/%d"), SelectedCount, MaxPartySize);
	TArray<SolidMenuPointer::FRow> Rows;
	SolidHUDPrivate::DrawChoicePopup(Canvas, Title, Labels, PartyBuilderIndex, Rows, &Enabled);
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
