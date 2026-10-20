#include "SolidWorldChartEdMode.h"

#include "Lairs/SolidLair.h"
#include "Strongholds/SolidStronghold.h"
#include "Terrain/SolidTerrainMap.h"
#include "Terrain/SolidTerrainStreamer.h"
#include "Terrain/SolidWorldMap.h"
#include "Towns/SolidTowns.h"

#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "Components/LineBatchComponent.h"
#include "Components/TextRenderComponent.h"
#include "EditorModeManager.h"
#include "EditorViewportClient.h"
#include "Engine/Engine.h"
#include "Engine/TextRenderActor.h"
#include "Engine/World.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "SceneView.h"
#include "UObject/SavePackage.h"

#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

const FEditorModeID FSolidWorldChartEdMode::Id(TEXT("EM_SolidWorldChart"));
const TCHAR* FSolidWorldChartEdMode::OpenWorldLevel = TEXT("/Game/ThirdPerson/Lvl_ThirdPerson");

namespace
{
	const TCHAR* BiomeName(const ESolidBiome Biome)
	{
		switch (Biome)
		{
		case ESolidBiome::Forest: return TEXT("Forest");
		case ESolidBiome::Mountain: return TEXT("Mountain");
		case ESolidBiome::Town: return TEXT("Town");
		case ESolidBiome::Desert: return TEXT("Desert");
		case ESolidBiome::Sea: return TEXT("Sea");
		case ESolidBiome::River: return TEXT("River");
		case ESolidBiome::Swamp: return TEXT("Swamp");
		case ESolidBiome::Road: return TEXT("Road");
		case ESolidBiome::Grassland:
		default: return TEXT("Grassland");
		}
	}

	const TCHAR* MarkKindName(const ESolidWorldChartMarkKind Kind)
	{
		switch (Kind)
		{
		case ESolidWorldChartMarkKind::Stronghold: return TEXT("Stronghold");
		case ESolidWorldChartMarkKind::StrongholdSite: return TEXT("Stronghold site");
		case ESolidWorldChartMarkKind::LairEntrance: return TEXT("Lair entrance");
		case ESolidWorldChartMarkKind::LairExit: return TEXT("Lair exit");
		case ESolidWorldChartMarkKind::Custom: return TEXT("Custom");
		case ESolidWorldChartMarkKind::Town:
		default: return TEXT("Town");
		}
	}

	FLinearColor MarkColor(const ESolidWorldChartMarkKind Kind)
	{
		switch (Kind)
		{
		case ESolidWorldChartMarkKind::Stronghold: return FLinearColor(0.95f, 0.22f, 0.18f);
		case ESolidWorldChartMarkKind::StrongholdSite: return FLinearColor(0.72f, 0.38f, 0.95f);
		case ESolidWorldChartMarkKind::LairEntrance: return FLinearColor(0.15f, 0.82f, 0.90f);
		case ESolidWorldChartMarkKind::LairExit: return FLinearColor(0.35f, 0.48f, 1.f);
		case ESolidWorldChartMarkKind::Custom: return FLinearColor(1.f, 0.35f, 0.75f);
		case ESolidWorldChartMarkKind::Town:
		default: return FLinearColor(1.f, 0.78f, 0.15f);
		}
	}

	bool HitGround(const USolidTerrainMap& Terrain, const FVector& Origin, const FVector& Direction, FVector& OutHit)
	{
		const FVector Dir = Direction.GetSafeNormal();
		if (Dir.IsNearlyZero())
		{
			return false;
		}

		const float Step = 200.f;
		const float MaxDist = 400000.f;
		bool bHasPrevious = false;
		float PreviousDelta = 0.f;
		FVector Previous = Origin;
		for (float Distance = 0.f; Distance <= MaxDist; Distance += Step)
		{
			const FVector Point = Origin + Dir * Distance;
			const float Delta = Point.Z - Terrain.SampleHeight(Point.X, Point.Y);
			if (bHasPrevious && PreviousDelta > 0.f && Delta <= 0.f)
			{
				const float Alpha = PreviousDelta / (PreviousDelta - Delta);
				OutHit = FMath::Lerp(Previous, Point, FMath::Clamp(Alpha, 0.f, 1.f));
				return true;
			}
			Previous = Point;
			PreviousDelta = Delta;
			bHasPrevious = true;
		}
		return false;
	}

	void CellCorners(
		const FIntPoint& Cell,
		const FVector2D& WorldMin,
		const FVector2D& WorldMax,
		FVector2D& OutWestSouth,
		FVector2D& OutEastNorth)
	{
		const float ExtentX = FMath::Max(WorldMax.X - WorldMin.X, 1.f);
		const float ExtentY = FMath::Max(WorldMax.Y - WorldMin.Y, 1.f);
		const float CellW = ExtentX / static_cast<float>(USolidWorldMap::MapWidth);
		const float CellH = ExtentY / static_cast<float>(USolidWorldMap::MapHeight);
		const float East = WorldMax.X - static_cast<float>(Cell.X) * CellW;
		const float North = WorldMax.Y - static_cast<float>(Cell.Y) * CellH;
		OutWestSouth = FVector2D(East - CellW, North - CellH);
		OutEastNorth = FVector2D(East, North);
	}
}

class FSolidWorldChartToolkit : public FModeToolkit
{
public:
	explicit FSolidWorldChartToolkit(FSolidWorldChartEdMode* InMode)
		: Mode(InMode)
	{
	}

	virtual FEdMode* GetEditorMode() const override { return Mode; }

	virtual TSharedPtr<SWidget> GetInlineContent() const override { return Panel; }

	virtual FName GetToolkitFName() const override { return TEXT("WorldChart"); }

	virtual FText GetBaseToolkitName() const override
	{
		return NSLOCTEXT("SolidWorldChart", "Toolkit", "World chart");
	}

	void Refresh()
	{
		if (Panel.IsValid())
		{
			Panel->Invalidate(EInvalidateWidgetReason::Layout);
		}
	}

	void BuildPanel()
	{
		Biomes = {
			ESolidBiome::Grassland,
			ESolidBiome::Forest,
			ESolidBiome::Mountain,
			ESolidBiome::Town,
			ESolidBiome::Desert,
			ESolidBiome::Sea,
			ESolidBiome::River
		};
		for (const SolidTowns::FTownInfo& Town : SolidTowns::Definitions)
		{
			Towns.Add(MakeShared<FString>(Town.Name));
		}
		for (const ESolidStrongholdSize Size : SolidStronghold::Sizes)
		{
			Sizes.Add(MakeShared<FString>(SolidStronghold::SizeName(Size)));
		}
		for (const ESolidStrongholdType Type : SolidStronghold::Types)
		{
			Types.Add(MakeShared<FString>(SolidStronghold::TypeName(Type)));
		}
		for (const SolidLair::FDefinition& Lair : SolidLair::Definitions)
		{
			Lairs.Add(MakeShared<FString>(Lair.Name));
		}
		MarkKinds.Add(MakeShared<FString>(TEXT("Town")));
		MarkKinds.Add(MakeShared<FString>(TEXT("Stronghold")));
		MarkKinds.Add(MakeShared<FString>(TEXT("Stronghold site")));
		MarkKinds.Add(MakeShared<FString>(TEXT("Lair entrance")));
		MarkKinds.Add(MakeShared<FString>(TEXT("Lair exit")));
		MarkKinds.Add(MakeShared<FString>(TEXT("Custom")));
		RoadTypes.Add(MakeShared<FString>(TEXT("Trail")));
		RoadTypes.Add(MakeShared<FString>(TEXT("Track")));
		RoadTypes.Add(MakeShared<FString>(TEXT("Road")));
		RoadTypes.Add(MakeShared<FString>(TEXT("Highway")));

		TSharedRef<SWrapBox> BrushRow = SNew(SWrapBox).UseAllottedSize(true);
		for (const ESolidBiome Biome : Biomes)
		{
			BrushRow->AddSlot()
			.Padding(2.f)
			[
				SNew(SButton)
				.Text(FText::FromString(BiomeName(Biome)))
				.ButtonColorAndOpacity_Lambda([this, Biome]()
				{
					return Mode && Mode->BrushBiome == Biome
						? FLinearColor(0.25f, 0.55f, 0.25f)
						: FLinearColor::White;
				})
				.OnClicked_Lambda([this, Biome]()
				{
					if (Mode)
					{
						Mode->SetBrush(Biome);
					}
					return FReply::Handled();
				})
			];
		}

		Panel = SNew(SBorder)
			.Padding(8.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
				[
					SNew(STextBlock)
					.AutoWrapText(true)
					.Text_Lambda([this]()
					{
						return FText::FromString(Mode ? Mode->BannerText() : FString());
					})
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f)
				[
					SNew(STextBlock).Text(NSLOCTEXT("SolidWorldChart", "Paint", "Paint"))
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					BrushRow
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 2.f)
				[
					SNew(STextBlock).Text(NSLOCTEXT("SolidWorldChart", "Mark", "Mark"))
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					MakeCombo(MarkKinds, [this]()
					{
						const int32 Index = Mode ? static_cast<int32>(Mode->MarkKind) : 0;
						return MarkKinds.IsValidIndex(Index) && MarkKinds[Index].IsValid()
							? *MarkKinds[Index]
							: FString();
					}, [this](const int32 Index)
					{
						if (Mode)
						{
							Mode->MarkKind = static_cast<ESolidWorldChartMarkKind>(Index);
							Mode->RefreshPanel();
						}
					})
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)
				[
					SNew(SBox)
					.Visibility_Lambda([this]()
					{
						return Mode && Mode->MarkKind == ESolidWorldChartMarkKind::Town
							? EVisibility::Visible
							: EVisibility::Collapsed;
					})
					[
						MakeCombo(Towns, [this]()
						{
							const int32 Index = Mode ? Mode->TownIndex : 0;
							return Towns.IsValidIndex(Index) && Towns[Index].IsValid() ? *Towns[Index] : FString();
						}, [this](const int32 Index)
						{
							if (Mode)
							{
								Mode->TownIndex = Index;
							}
						})
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)
				[
					SNew(SBox)
					.Visibility_Lambda([this]()
					{
						const bool bSize = Mode && (Mode->MarkKind == ESolidWorldChartMarkKind::Stronghold
							|| Mode->MarkKind == ESolidWorldChartMarkKind::StrongholdSite);
						return bSize ? EVisibility::Visible : EVisibility::Collapsed;
					})
					[
						MakeCombo(Sizes, [this]()
						{
							const int32 Index = Mode ? Mode->SizeIndex : 0;
							return Sizes.IsValidIndex(Index) && Sizes[Index].IsValid() ? *Sizes[Index] : FString();
						}, [this](const int32 Index)
						{
							if (Mode)
							{
								Mode->SizeIndex = Index;
							}
						})
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)
				[
					SNew(SBox)
					.Visibility_Lambda([this]()
					{
						return Mode && Mode->MarkKind == ESolidWorldChartMarkKind::Stronghold
							? EVisibility::Visible
							: EVisibility::Collapsed;
					})
					[
						MakeCombo(Types, [this]()
						{
							const int32 Index = Mode ? Mode->TypeIndex : 0;
							return Types.IsValidIndex(Index) && Types[Index].IsValid() ? *Types[Index] : FString();
						}, [this](const int32 Index)
						{
							if (Mode)
							{
								Mode->TypeIndex = Index;
							}
						})
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)
				[
					SNew(SBox)
					.Visibility_Lambda([this]()
					{
						const bool bLair = Mode && (Mode->MarkKind == ESolidWorldChartMarkKind::LairEntrance
							|| Mode->MarkKind == ESolidWorldChartMarkKind::LairExit);
						return bLair ? EVisibility::Visible : EVisibility::Collapsed;
					})
					[
						MakeCombo(Lairs, [this]()
						{
							const int32 Index = Mode ? Mode->LairIndex : 0;
							return Lairs.IsValidIndex(Index) && Lairs[Index].IsValid() ? *Lairs[Index] : FString();
						}, [this](const int32 Index)
						{
							if (Mode)
							{
								Mode->LairIndex = Index;
							}
						})
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)
				[
					SNew(SBox)
					.Visibility_Lambda([this]()
					{
						return Mode && Mode->MarkKind == ESolidWorldChartMarkKind::Custom
							? EVisibility::Visible
							: EVisibility::Collapsed;
					})
					[
						SNew(SEditableTextBox)
						.HintText(NSLOCTEXT("SolidWorldChart", "NoteHint", "A short note"))
						.Text_Lambda([this]()
						{
							return FText::FromString(Mode ? Mode->CustomNote : FString());
						})
						.OnTextChanged_Lambda([this](const FText& Text)
						{
							if (Mode)
							{
								Mode->CustomNote = Text.ToString();
							}
						})
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 4.f, 0.f)
					[
						SNew(SButton)
						.Text(NSLOCTEXT("SolidWorldChart", "Place", "Place"))
						.OnClicked_Lambda([this]()
						{
							if (Mode)
							{
								Mode->PlaceCurrentMark();
							}
							return FReply::Handled();
						})
					]
					+ SHorizontalBox::Slot().FillWidth(1.f)
					[
						SNew(SButton)
						.Text(NSLOCTEXT("SolidWorldChart", "Delete", "Delete"))
						.OnClicked_Lambda([this]()
						{
							if (Mode)
							{
								Mode->DeleteCurrentMark();
							}
							return FReply::Handled();
						})
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 2.f)
				[
					SNew(STextBlock).Text(NSLOCTEXT("SolidWorldChart", "Road", "Road"))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
				[
					SNew(SEditableTextBox)
					.HintText(NSLOCTEXT("SolidWorldChart", "From", "From"))
					.OnTextChanged_Lambda([this](const FText& Text)
					{
						if (Mode)
						{
							Mode->RoadFrom = Text.ToString();
							Mode->RoadNamesChanged();
						}
					})
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
				[
					SNew(SEditableTextBox)
					.HintText(NSLOCTEXT("SolidWorldChart", "To", "To"))
					.OnTextChanged_Lambda([this](const FText& Text)
					{
						if (Mode)
						{
							Mode->RoadTo = Text.ToString();
							Mode->RoadNamesChanged();
						}
					})
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
				[
					MakeCombo(RoadTypes, [this]()
					{
						const int32 Index = Mode ? static_cast<int32>(Mode->RoadType) : 0;
						return RoadTypes.IsValidIndex(Index) && RoadTypes[Index].IsValid()
							? *RoadTypes[Index]
							: FString();
					}, [this](const int32 Index)
					{
						if (Mode)
						{
							Mode->RoadType = static_cast<ESolidWorldChartRoadType>(Index);
						}
					})
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)
				[
					SNew(SButton)
					.Text(NSLOCTEXT("SolidWorldChart", "AddRoad", "Add road"))
					.OnClicked_Lambda([this]()
					{
						if (Mode)
						{
							Mode->AddCurrentRoad();
						}
						return FReply::Handled();
					})
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
				[
					SNew(STextBlock)
					.AutoWrapText(true)
					.Text_Lambda([this]()
					{
						return FText::FromString(Mode ? Mode->RoadLine() : FString());
					})
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f)
				[
					SNew(SSeparator)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f)
				[
					SNew(STextBlock)
					.AutoWrapText(true)
					.Text_Lambda([this]()
					{
						return FText::FromString(Mode ? Mode->HoverLine() : FString());
					})
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f)
				[
					SNew(STextBlock)
					.AutoWrapText(true)
					.Text_Lambda([this]()
					{
						return FText::FromString(Mode ? Mode->DifferenceText() : FString());
					})
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f)
				[
					SNew(STextBlock)
					.AutoWrapText(true)
					.Text_Lambda([this]()
					{
						return FText::FromString(Mode ? Mode->ToolLine() : FString());
					})
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 4.f, 0.f)
					[
						SNew(SButton)
						.Text(NSLOCTEXT("SolidWorldChart", "Undo", "Undo"))
						.OnClicked_Lambda([this]()
						{
							if (Mode)
							{
								Mode->UndoChart();
							}
							return FReply::Handled();
						})
					]
					+ SHorizontalBox::Slot().FillWidth(1.f)
					[
						SNew(SButton)
						.Text(NSLOCTEXT("SolidWorldChart", "Save", "Save"))
						.OnClicked_Lambda([this]()
						{
							if (Mode)
							{
								Mode->SaveChart();
							}
							return FReply::Handled();
						})
					]
				]
			];
	}

private:
	using FPick = TFunction<void(int32)>;

	TSharedRef<SWidget> MakeCombo(
		TArray<TSharedPtr<FString>>& Options,
		TFunction<FString()> Current,
		FPick Pick)
	{
		return SNew(SComboBox<TSharedPtr<FString>>)
			.OptionsSource(&Options)
			.OnSelectionChanged_Lambda([Options, Pick](TSharedPtr<FString> Item, ESelectInfo::Type)
			{
				const int32 Index = Options.IndexOfByKey(Item);
				if (Index != INDEX_NONE)
				{
					Pick(Index);
				}
			})
			.OnGenerateWidget_Lambda([](TSharedPtr<FString> Item)
			{
				return SNew(STextBlock).Text(FText::FromString(Item.IsValid() ? *Item : FString()));
			})
			[
				SNew(STextBlock)
				.Text_Lambda([Current]()
				{
					return FText::FromString(Current());
				})
			];
	}

	FSolidWorldChartEdMode* Mode = nullptr;
	TSharedPtr<SWidget> Panel;
	TArray<ESolidBiome> Biomes;
	TArray<TSharedPtr<FString>> Towns;
	TArray<TSharedPtr<FString>> Sizes;
	TArray<TSharedPtr<FString>> Types;
	TArray<TSharedPtr<FString>> Lairs;
	TArray<TSharedPtr<FString>> MarkKinds;
	TArray<TSharedPtr<FString>> RoadTypes;
};

FSolidWorldChartEdMode::FSolidWorldChartEdMode()
{
}

FSolidWorldChartEdMode::~FSolidWorldChartEdMode()
{
}

void FSolidWorldChartEdMode::Enter()
{
	FEdMode::Enter();
	Chart = LoadObject<USolidWorldChart>(nullptr, USolidWorldChart::ShippedPath());
	ShowChart();
	RefreshDisagreement();
	RefreshRoadMessage();
	Hover = TEXT("Hover a cell.");

	ChartToolkit = MakeShared<FSolidWorldChartToolkit>(this);
	ChartToolkit->BuildPanel();
	Toolkit = ChartToolkit;
	if (Owner && Owner->GetToolkitHost().IsValid())
	{
		// Init subscribes to the mode change. The editor registers this toolkit
		// once, after Enter returns. Registering it here as well asserts.
		ChartToolkit->Init(Owner->GetToolkitHost());
	}
}

void FSolidWorldChartEdMode::Exit()
{
	ClearChartActors();
	// The mode manager already closed this toolkit when the mode was deactivated.
	Toolkit.Reset();
	ChartToolkit.Reset();
	Chart = nullptr;
	FEdMode::Exit();
}

void FSolidWorldChartEdMode::Tick(FEditorViewportClient* ViewportClient, float DeltaTime)
{
	FEdMode::Tick(ViewportClient, DeltaTime);
	if (ViewportClient)
	{
		FaceSigns(ViewportClient->GetViewLocation());
	}
}

void FSolidWorldChartEdMode::Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI)
{
	FEdMode::Render(View, Viewport, PDI);
	if (View)
	{
		FaceSigns(View->ViewLocation);
	}
	const USolidTerrainMap* Terrain = Streamer ? Streamer->GetTerrainMap() : nullptr;
	if (!PDI || !Terrain || !Terrain->IsBuilt() || !bStroke)
	{
		return;
	}

	const FLinearColor Color = USolidWorldMap::DefaultColorForBiome(BrushBiome);
	const FVector2D WorldMin = Terrain->GetWorldMinXY();
	const FVector2D WorldMax = Terrain->GetWorldMaxXY();
	for (const FIntPoint& Cell : PreviewCells())
	{
		FVector2D WestSouth = FVector2D::ZeroVector;
		FVector2D EastNorth = FVector2D::ZeroVector;
		CellCorners(Cell, WorldMin, WorldMax, WestSouth, EastNorth);
		const float Lift = 140.f;
		const FVector A(WestSouth.X, WestSouth.Y, Terrain->SampleHeight(WestSouth.X, WestSouth.Y) + Lift);
		const FVector B(EastNorth.X, WestSouth.Y, Terrain->SampleHeight(EastNorth.X, WestSouth.Y) + Lift);
		const FVector C(EastNorth.X, EastNorth.Y, Terrain->SampleHeight(EastNorth.X, EastNorth.Y) + Lift);
		const FVector D(WestSouth.X, EastNorth.Y, Terrain->SampleHeight(WestSouth.X, EastNorth.Y) + Lift);
		PDI->DrawLine(A, B, Color, SDPG_Foreground, 3.f);
		PDI->DrawLine(B, C, Color, SDPG_Foreground, 3.f);
		PDI->DrawLine(C, D, Color, SDPG_Foreground, 3.f);
		PDI->DrawLine(D, A, Color, SDPG_Foreground, 3.f);
	}
}

bool FSolidWorldChartEdMode::InputKey(
	FEditorViewportClient* ViewportClient,
	FViewport* Viewport,
	FKey Key,
	EInputEvent Event)
{
	if (Key == EKeys::Z && Event == IE_Pressed && ViewportClient && ViewportClient->IsCtrlPressed())
	{
		UndoChart();
		return true;
	}
	// Alt copies the biome on hover and orbits the camera. It does not paint.
	if (Key == EKeys::LeftMouseButton && ViewportClient && Viewport && !ViewportClient->IsAltPressed())
	{
		if (Event == IE_Pressed)
		{
			BeginStroke(ViewportClient, Viewport);
			return true;
		}
		if (Event == IE_Released)
		{
			EndStroke();
			return true;
		}
	}
	return false;
}

bool FSolidWorldChartEdMode::MouseMove(
	FEditorViewportClient* ViewportClient,
	FViewport* Viewport,
	int32 X,
	int32 Y)
{
	HoverAt(ViewportClient, Viewport, X, Y);
	if (!bStroke)
	{
		return false;
	}
	ExtendStroke(ViewportClient, Viewport, X, Y);
	return true;
}

bool FSolidWorldChartEdMode::CapturedMouseMove(
	FEditorViewportClient* ViewportClient,
	FViewport* Viewport,
	int32 X,
	int32 Y)
{
	if (!bStroke)
	{
		return false;
	}
	return MouseMove(ViewportClient, Viewport, X, Y);
}

bool FSolidWorldChartEdMode::StartTracking(FEditorViewportClient* ViewportClient, FViewport* Viewport)
{
	// Right-mouse look, middle-mouse pan, and Alt orbit also start tracking.
	// A stroke is the left button only, and InputKey already began it.
	if (!Viewport || !Viewport->KeyState(EKeys::LeftMouseButton) || (ViewportClient && ViewportClient->IsAltPressed()))
	{
		return false;
	}
	BeginStroke(ViewportClient, Viewport);
	return bStroke;
}

bool FSolidWorldChartEdMode::EndTracking(FEditorViewportClient* ViewportClient, FViewport* Viewport)
{
	if (!bStroke)
	{
		return false;
	}
	EndStroke();
	return true;
}

bool FSolidWorldChartEdMode::HandleClick(
	FEditorViewportClient* ViewportClient,
	HHitProxy* HitProxy,
	const FViewportClick& Click)
{
	return true;
}

void FSolidWorldChartEdMode::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(Chart);
	Collector.AddReferencedObject(Streamer);
	Collector.AddReferencedObject(Overlay);
	Collector.AddReferencedObject(Lines);
	Collector.AddReferencedObjects(Signs);
}

void FSolidWorldChartEdMode::SetBrush(const ESolidBiome Biome)
{
	BrushBiome = Biome;
	RefreshPanel();
}

void FSolidWorldChartEdMode::RoadNamesChanged()
{
	RefreshRoadMessage();
	RefreshPanel();
}

void FSolidWorldChartEdMode::PlaceCurrentMark()
{
	if (!Chart)
	{
		Note(TEXT("The chart is missing."));
		return;
	}
	if (!bHasCell)
	{
		Note(TEXT("Hover a cell, then place the mark."));
		return;
	}

	FString Error;
	bool bPlaced = false;
	switch (MarkKind)
	{
	case ESolidWorldChartMarkKind::Town:
		if (SolidTowns::Definitions[FMath::Clamp(TownIndex, 0, SolidTowns::DefinitionCount - 1)].Name)
		{
			bPlaced = Chart->PlaceTown(
				CurrentCell,
				SolidTowns::Definitions[FMath::Clamp(TownIndex, 0, SolidTowns::DefinitionCount - 1)].Name,
				Error);
		}
		break;
	case ESolidWorldChartMarkKind::Stronghold:
		bPlaced = Chart->PlaceStronghold(
			CurrentCell,
			SolidStronghold::Sizes[FMath::Clamp(SizeIndex, 0, SolidStronghold::SizeCount - 1)],
			SolidStronghold::Types[FMath::Clamp(TypeIndex, 0, SolidStronghold::TypeCount - 1)],
			Error);
		break;
	case ESolidWorldChartMarkKind::StrongholdSite:
		bPlaced = Chart->PlaceSite(
			CurrentCell,
			SolidStronghold::Sizes[FMath::Clamp(SizeIndex, 0, SolidStronghold::SizeCount - 1)],
			Error);
		break;
	case ESolidWorldChartMarkKind::LairEntrance:
	case ESolidWorldChartMarkKind::LairExit:
		if (SolidLair::DefinitionCount > 0)
		{
			bPlaced = Chart->PlaceLairMouth(
				CurrentCell,
				SolidLair::Definitions[FMath::Clamp(LairIndex, 0, SolidLair::DefinitionCount - 1)].Name,
				MarkKind == ESolidWorldChartMarkKind::LairEntrance,
				Error);
		}
		break;
	case ESolidWorldChartMarkKind::Custom:
		bPlaced = Chart->PlaceCustom(CurrentCell, CustomNote, Error);
		break;
	default:
		break;
	}

	if (!bPlaced)
	{
		Note(Error.IsEmpty() ? TEXT("The mark was not placed.") : Error);
		return;
	}

	Note(FString::Printf(TEXT("Placed %s."), MarkKindName(MarkKind)));
	if (Streamer)
	{
		Streamer->RebuildChartView(Chart, { CurrentCell });
	}
	RebuildOverlay();
	RebuildSigns();
	RefreshDisagreement();
	Hover = DescribeCell(CurrentCell);
	RefreshPanel();
}

void FSolidWorldChartEdMode::DeleteCurrentMark()
{
	if (!Chart || !bHasCell)
	{
		Note(TEXT("Hover a mark, then delete it."));
		return;
	}
	FString Error;
	if (!Chart->DeleteMark(CurrentCell, Error))
	{
		Note(Error);
		return;
	}
	Note(TEXT("Deleted the mark."));
	if (Streamer)
	{
		Streamer->RebuildChartView(Chart, { CurrentCell });
	}
	RebuildOverlay();
	RebuildSigns();
	RefreshDisagreement();
	Hover = DescribeCell(CurrentCell);
	RefreshPanel();
}

void FSolidWorldChartEdMode::AddCurrentRoad()
{
	if (!Chart)
	{
		Note(TEXT("The chart is missing."));
		return;
	}
	FString Error;
	if (!Chart->AddRoad(RoadFrom, RoadTo, RoadType, Error))
	{
		Note(Error);
		RefreshRoadMessage();
		RefreshPanel();
		return;
	}

	TArray<FIntPoint> Route;
	Chart->RouteForRoad(Chart->Roads.Num() - 1, Route);
	if (Streamer)
	{
		if (Route.Num() > 0)
		{
			Streamer->RebuildChartView(Chart, Route);
		}
	}
	RoadMessage = Error;
	Note(Error.IsEmpty()
		? FString::Printf(TEXT("Added %s."), USolidWorldChart::RoadTypeName(RoadType))
		: Error);
	RefreshDisagreement();
	RefreshPanel();
}

void FSolidWorldChartEdMode::UndoChart()
{
	if (!Chart || !Chart->Undo())
	{
		Note(TEXT("Nothing to undo."));
		return;
	}
	Note(TEXT("Undone."));
	if (Streamer)
	{
		Streamer->BuildChartView(Chart);
	}
	RebuildOverlay();
	RebuildSigns();
	RefreshDisagreement();
	RefreshRoadMessage();
	if (bHasCell)
	{
		Hover = DescribeCell(CurrentCell);
	}
	RefreshPanel();
}

void FSolidWorldChartEdMode::SaveChart()
{
	if (!Chart)
	{
		Note(TEXT("The chart is missing."));
		return;
	}

	UPackage* Package = Chart->GetPackage();
	Chart->MarkPackageDirty();
	const FString Filename = FPackageName::LongPackageNameToFilename(
		Package->GetName(),
		FPackageName::GetAssetPackageExtension());
	FSavePackageArgs Args;
	Args.TopLevelFlags = RF_Public | RF_Standalone;
	if (UPackage::SavePackage(Package, Chart, *Filename, Args))
	{
		Note(TEXT("Saved /Game/World/WorldChart."));
	}
	else
	{
		Note(TEXT("Save failed."));
	}
	RefreshPanel();
}

void FSolidWorldChartEdMode::RefreshPanel()
{
	if (ChartToolkit.IsValid())
	{
		ChartToolkit->Refresh();
	}
}

bool FSolidWorldChartEdMode::IsOpenWorld(const UWorld* World) const
{
	return World && World->GetOutermost()->GetName() == OpenWorldLevel;
}

bool FSolidWorldChartEdMode::CursorCell(
	FEditorViewportClient* ViewportClient,
	FViewport* Viewport,
	int32 X,
	int32 Y,
	FIntPoint& OutCell) const
{
	const USolidTerrainMap* Terrain = Streamer ? Streamer->GetTerrainMap() : nullptr;
	const USolidWorldMap* WorldMap = Terrain ? Terrain->GetWorldMap() : nullptr;
	if (!ViewportClient || !Viewport || !Terrain || !Terrain->IsBuilt() || !WorldMap)
	{
		return false;
	}

	FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(
		Viewport,
		ViewportClient->GetScene(),
		ViewportClient->EngineShowFlags)
		.SetRealtimeUpdate(false));
	FSceneView* View = ViewportClient->CalcSceneView(&ViewFamily);
	if (!View)
	{
		return false;
	}

	FVector Origin = FVector::ZeroVector;
	FVector Direction = FVector::ZeroVector;
	View->DeprojectFVector2D(FVector2D(static_cast<float>(X), static_cast<float>(Y)), Origin, Direction);
	FVector Hit = FVector::ZeroVector;
	if (!HitGround(*Terrain, Origin, Direction, Hit))
	{
		return false;
	}
	return WorldMap->CellAtWorld(
		Hit.X,
		Hit.Y,
		Terrain->GetWorldMinXY(),
		Terrain->GetWorldMaxXY(),
		OutCell);
}

void FSolidWorldChartEdMode::HoverAt(
	FEditorViewportClient* ViewportClient,
	FViewport* Viewport,
	int32 X,
	int32 Y)
{
	FIntPoint Cell = FIntPoint::ZeroValue;
	if (!CursorCell(ViewportClient, Viewport, X, Y, Cell))
	{
		return;
	}
	bHasCell = true;
	CurrentCell = Cell;
	if (ViewportClient && ViewportClient->IsAltPressed())
	{
		SampleBrush(Cell);
	}
	const FString Next = DescribeCell(Cell);
	if (Next != Hover)
	{
		Hover = Next;
		RefreshPanel();
	}
}

void FSolidWorldChartEdMode::BeginStroke(FEditorViewportClient* ViewportClient, FViewport* Viewport)
{
	if (bStroke || !ViewportClient || !Viewport)
	{
		return;
	}
	const int32 X = Viewport->GetMouseX();
	const int32 Y = Viewport->GetMouseY();
	FIntPoint Cell = FIntPoint::ZeroValue;
	if (!CursorCell(ViewportClient, Viewport, X, Y, Cell))
	{
		return;
	}
	bHasCell = true;
	CurrentCell = Cell;
	if (ViewportClient->IsAltPressed())
	{
		SampleBrush(Cell);
		return;
	}
	bStroke = true;
	bRectangle = ViewportClient->IsShiftPressed();
	Anchor = Cell;
	StrokeCells.Reset();
	StrokeCells.Add(Cell);
}

void FSolidWorldChartEdMode::ExtendStroke(
	FEditorViewportClient* ViewportClient,
	FViewport* Viewport,
	int32 X,
	int32 Y)
{
	if (!bStroke || !ViewportClient)
	{
		return;
	}
	if (ViewportClient->IsAltPressed())
	{
		bStroke = false;
		StrokeCells.Reset();
		FIntPoint Cell = FIntPoint::ZeroValue;
		if (CursorCell(ViewportClient, Viewport, X, Y, Cell))
		{
			SampleBrush(Cell);
		}
		return;
	}

	FIntPoint Cell = FIntPoint::ZeroValue;
	if (!CursorCell(ViewportClient, Viewport, X, Y, Cell))
	{
		return;
	}
	CurrentCell = Cell;
	bRectangle = ViewportClient->IsShiftPressed();
	if (!StrokeCells.Contains(Cell))
	{
		StrokeCells.Add(Cell);
	}
}

void FSolidWorldChartEdMode::EndStroke()
{
	if (!bStroke)
	{
		return;
	}
	bStroke = false;
	if (!Chart)
	{
		StrokeCells.Reset();
		return;
	}

	FString Error;
	const bool bPainted = bRectangle
		? Chart->PaintRectangle(Anchor, CurrentCell, BrushBiome, Error)
		: Chart->PaintRun(StrokeCells, BrushBiome, Error);
	const TArray<FIntPoint> Painted = bRectangle ? PreviewCells() : StrokeCells;
	StrokeCells.Reset();
	if (!bPainted)
	{
		Note(Error);
		RefreshPanel();
		return;
	}

	if (Streamer)
	{
		Streamer->RebuildChartView(Chart, Painted);
	}
	RebuildOverlay();
	RefreshDisagreement();
	if (bHasCell)
	{
		Hover = DescribeCell(CurrentCell);
	}
	Note(TEXT("Painted."));
	RefreshPanel();
}

void FSolidWorldChartEdMode::SampleBrush(const FIntPoint& Cell)
{
	if (!Chart)
	{
		return;
	}
	const int32 Index = Cell.Y * USolidWorldMap::MapWidth + Cell.X;
	if (!Chart->Cells.IsValidIndex(Index))
	{
		return;
	}
	const ESolidBiome Painted = Chart->Cells[Index];
	if (Painted == ESolidBiome::Swamp || Painted == ESolidBiome::Road)
	{
		return;
	}
	SetBrush(Painted);
	Note(FString::Printf(TEXT("Brush is %s."), BiomeName(Painted)));
}

void FSolidWorldChartEdMode::ShowChart()
{
	UWorld* World = GetWorld();
	if (!Chart)
	{
		Banner = TEXT("Missing /Game/World/WorldChart. Import WorldMap.txt into that asset first.");
		return;
	}
	if (!IsOpenWorld(World))
	{
		Banner = TEXT("Open /Game/ThirdPerson/Lvl_ThirdPerson to paint the chart.");
		return;
	}

	Banner.Reset();
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.ObjectFlags = RF_Transient;
	Params.bHideFromSceneOutliner = true;
	Streamer = World->SpawnActor<ASolidTerrainStreamer>(
		ASolidTerrainStreamer::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
	if (!Streamer)
	{
		Banner = TEXT("The chart terrain did not spawn.");
		return;
	}
	Streamer->bIsEditorOnlyActor = true;
	Streamer->BuildChartView(Chart);

	Overlay = World->SpawnActor<AActor>(
		AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
	if (Overlay)
	{
		Overlay->bIsEditorOnlyActor = true;
		Lines = NewObject<ULineBatchComponent>(Overlay, TEXT("ChartLines"));
		Overlay->AddInstanceComponent(Lines);
		Overlay->SetRootComponent(Lines);
		Lines->RegisterComponent();
	}
	RebuildOverlay();
	RebuildSigns();
}

void FSolidWorldChartEdMode::ClearChartActors()
{
	for (const TObjectPtr<ATextRenderActor>& Sign : Signs)
	{
		if (Sign)
		{
			Sign->Destroy();
		}
	}
	Signs.Reset();
	if (Overlay)
	{
		Overlay->Destroy();
		Overlay = nullptr;
		Lines = nullptr;
	}
	if (Streamer)
	{
		Streamer->ClearChartView();
		Streamer->Destroy();
		Streamer = nullptr;
	}
}

void FSolidWorldChartEdMode::RebuildOverlay()
{
	if (!Lines || !Chart || !Streamer || !Streamer->GetTerrainMap())
	{
		return;
	}
	const USolidTerrainMap* Terrain = Streamer->GetTerrainMap();
	const USolidWorldMap* WorldMap = Terrain->GetWorldMap();
	if (!Terrain->IsBuilt() || !WorldMap)
	{
		return;
	}

	Lines->Flush();
	const FVector2D WorldMin = Terrain->GetWorldMinXY();
	const FVector2D WorldMax = Terrain->GetWorldMaxXY();
	TArray<FBatchedLine> Batch;
	Batch.Reserve(USolidWorldMap::MapWidth * USolidWorldMap::MapHeight * 4);
	const float Lift = 80.f;
	for (int32 Row = 0; Row < USolidWorldMap::MapHeight; ++Row)
	{
		for (int32 Col = 0; Col < USolidWorldMap::MapWidth; ++Col)
		{
			const FIntPoint Cell(Col, Row);
			FVector2D WestSouth = FVector2D::ZeroVector;
			FVector2D EastNorth = FVector2D::ZeroVector;
			CellCorners(Cell, WorldMin, WorldMax, WestSouth, EastNorth);
			const FLinearColor Color = WorldMap->GetBiomeColor(WorldMap->GetBiomeAtCell(Col, Row));
			const FVector A(WestSouth.X, WestSouth.Y, Terrain->SampleHeight(WestSouth.X, WestSouth.Y) + Lift);
			const FVector B(EastNorth.X, WestSouth.Y, Terrain->SampleHeight(EastNorth.X, WestSouth.Y) + Lift);
			const FVector C(EastNorth.X, EastNorth.Y, Terrain->SampleHeight(EastNorth.X, EastNorth.Y) + Lift);
			const FVector D(WestSouth.X, EastNorth.Y, Terrain->SampleHeight(WestSouth.X, EastNorth.Y) + Lift);
			Batch.Emplace(A, B, Color, 0.f, 1.5f, SDPG_World);
			Batch.Emplace(B, C, Color, 0.f, 1.5f, SDPG_World);
			Batch.Emplace(C, D, Color, 0.f, 1.5f, SDPG_World);
			Batch.Emplace(D, A, Color, 0.f, 1.5f, SDPG_World);
		}
	}
	Lines->DrawLines(Batch);
}

void FSolidWorldChartEdMode::RebuildSigns()
{
	UWorld* World = GetWorld();
	const USolidTerrainMap* Terrain = Streamer ? Streamer->GetTerrainMap() : nullptr;
	for (const TObjectPtr<ATextRenderActor>& Sign : Signs)
	{
		if (Sign)
		{
			Sign->Destroy();
		}
	}
	Signs.Reset();
	if (!World || !Chart || !Terrain || !Terrain->IsBuilt())
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.ObjectFlags = RF_Transient;
	Params.bHideFromSceneOutliner = true;
	for (const FSolidWorldChartMark& Mark : Chart->Marks)
	{
		FVector Location = FVector::ZeroVector;
		if (!MarkLocation(Mark, Location))
		{
			continue;
		}
		ATextRenderActor* Sign = World->SpawnActor<ATextRenderActor>(
			ATextRenderActor::StaticClass(), Location, FRotator::ZeroRotator, Params);
		if (!Sign)
		{
			continue;
		}
		Sign->bIsEditorOnlyActor = true;
		if (UTextRenderComponent* Text = Sign->GetTextRender())
		{
			Text->SetText(FText::FromString(Mark.Name));
			Text->SetWorldSize(1200.f);
			Text->SetHorizontalAlignment(EHTA_Center);
			Text->SetVerticalAlignment(EVRTA_TextCenter);
			Text->SetTextRenderColor(FColor::White);
			Text->LDMaxDrawDistance = 0.f;
			Text->bNeverDistanceCull = true;
			if (GEngine && GEngine->GetMediumFont())
			{
				Text->SetFont(GEngine->GetMediumFont());
			}
		}
		Signs.Add(Sign);
	}
}

void FSolidWorldChartEdMode::FaceSigns(const FVector& Camera)
{
	// Close up, a name stays 12 m tall. Farther away it stays about two percent of the
	// distance tall, and turns to face the camera so a top-down view can still read it.
	constexpr float CloseHeightCm = 1200.f;
	constexpr float FarHeightOverDistance = 0.02f;
	for (const TObjectPtr<ATextRenderActor>& Sign : Signs)
	{
		if (!Sign || Sign->IsTemporarilyHiddenInEditor())
		{
			continue;
		}
		const FVector ToCamera = Camera - Sign->GetActorLocation();
		if (ToCamera.IsNearlyZero())
		{
			continue;
		}
		const FRotator Facing = FRotationMatrix::MakeFromX(ToCamera).Rotator();
		if (!Sign->GetActorRotation().Equals(Facing, 0.5f))
		{
			Sign->SetActorRotation(Facing);
		}
		const float Height = FMath::Max(CloseHeightCm, ToCamera.Size() * FarHeightOverDistance);
		const FVector Scale(Height / CloseHeightCm);
		if (!Sign->GetActorScale3D().Equals(Scale, 0.02f))
		{
			Sign->SetActorScale3D(Scale);
		}
	}
}

bool FSolidWorldChartEdMode::MarkLocation(const FSolidWorldChartMark& Mark, FVector& OutLocation) const
{
	const USolidTerrainMap* Terrain = Streamer ? Streamer->GetTerrainMap() : nullptr;
	if (!Terrain || !Terrain->IsBuilt())
	{
		return false;
	}
	FVector2D ChartMin = FVector2D::ZeroVector;
	FVector2D ChartMax = FVector2D::ZeroVector;
	USolidWorldChart::DefaultWorldBounds(ChartMin, ChartMax);
	const FVector2D XY = Mark.PositionCm + (Terrain->GetWorldMinXY() - ChartMin);
	OutLocation = FVector(XY.X, XY.Y, Terrain->SampleHeight(XY.X, XY.Y) + 800.f);
	return true;
}

void FSolidWorldChartEdMode::DrawHUD(
	FEditorViewportClient* ViewportClient,
	FViewport* Viewport,
	const FSceneView* View,
	FCanvas* Canvas)
{
	FEdMode::DrawHUD(ViewportClient, Viewport, View, Canvas);
	if (!View || !Canvas || Canvas->IsHitTesting() || !Chart || !GEngine || !GEngine->GetSmallFont())
	{
		return;
	}
	UFont* Font = GEngine->GetSmallFont();
	const FIntRect ViewRect = View->UnscaledViewRect;
	const bool bSignsMatch = Signs.Num() == Chart->Marks.Num();
	const FVector2D Mouse(Viewport ? Viewport->GetMouseX() : -1.f, Viewport ? Viewport->GetMouseY() : -1.f);
	constexpr float HoverRadiusPx = 20.f;
	int32 HoveredIndex = INDEX_NONE;
	float HoveredDistSq = HoverRadiusPx * HoverRadiusPx;
	FVector2D HoveredPixel = FVector2D::ZeroVector;
	for (int32 Index = 0; Index < Chart->Marks.Num(); ++Index)
	{
		const FSolidWorldChartMark& Mark = Chart->Marks[Index];
		FVector Location = FVector::ZeroVector;
		if (!MarkLocation(Mark, Location) || Mark.Name.IsEmpty())
		{
			continue;
		}
		const FVector4 Screen = View->WorldToScreen(Location);
		if (Screen.W <= 0.f)
		{
			continue;
		}
		FVector2D Pixel;
		if (!View->ScreenToPixel(Screen, Pixel))
		{
			continue;
		}
		if (Pixel.X < ViewRect.Min.X - 80.f || Pixel.X > ViewRect.Max.X + 80.f
			|| Pixel.Y < ViewRect.Min.Y - 40.f || Pixel.Y > ViewRect.Max.Y + 80.f)
		{
			continue;
		}

		// The in-world name is 12 m tall. Farther out, the pin stands in for it.
		FVector2D Above = Pixel;
		const FVector4 AboveScreen = View->WorldToScreen(Location + FVector(0.f, 0.f, 1200.f));
		float Projected = 0.f;
		if (AboveScreen.W > 0.f && View->ScreenToPixel(AboveScreen, Above))
		{
			Projected = static_cast<float>(FVector2D::Distance(Pixel, Above));
		}
		const bool bFar = Projected < 160.f;
		if (bSignsMatch && Signs[Index] && Signs[Index]->IsTemporarilyHiddenInEditor() != bFar)
		{
			Signs[Index]->SetIsTemporarilyHiddenInEditor(bFar);
		}
		if (!bFar)
		{
			continue;
		}
		// Screen tiles, not scene points. Two scene points on one spot shimmer.
		constexpr float Outer = 18.f;
		constexpr float Inner = 10.f;
		FCanvasTileItem Halo(
			Pixel - FVector2D(Outer * 0.5f, Outer * 0.5f),
			FVector2D(Outer, Outer),
			FLinearColor::Black);
		Halo.BlendMode = SE_BLEND_Opaque;
		Canvas->DrawItem(Halo);
		FCanvasTileItem Core(
			Pixel - FVector2D(Inner * 0.5f, Inner * 0.5f),
			FVector2D(Inner, Inner),
			MarkColor(Mark.Kind));
		Core.BlendMode = SE_BLEND_Opaque;
		Canvas->DrawItem(Core);
		const float DistSq = static_cast<float>(FVector2D::DistSquared(Pixel, Mouse));
		if (DistSq <= HoveredDistSq)
		{
			HoveredDistSq = DistSq;
			HoveredIndex = Index;
			HoveredPixel = Pixel;
		}
	}

	if (HoveredIndex == INDEX_NONE)
	{
		return;
	}
	const FSolidWorldChartMark& Hovered = Chart->Marks[HoveredIndex];
	FCanvasTextItem Label(
		FVector2D(HoveredPixel.X, HoveredPixel.Y - 20.f),
		FText::FromString(Hovered.Name),
		Font,
		FLinearColor::White);
	Label.EnableShadow(FLinearColor::Black, FVector2D(1.5f, 1.5f));
	Label.bCentreX = true;
	Label.bOutlined = true;
	Label.OutlineColor = FLinearColor::Black;
	Label.Scale = FVector2D(1.8f);
	Canvas->DrawItem(Label);
}

void FSolidWorldChartEdMode::RefreshDisagreement()
{
	if (!Chart)
	{
		Difference.Reset();
		return;
	}
	const FString Path = FPaths::ProjectContentDir() / TEXT("WorldMap.txt");
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *Path))
	{
		Difference = TEXT("Content/WorldMap.txt did not load.");
		return;
	}
	Difference = Chart->Disagreement(Text);
}

void FSolidWorldChartEdMode::RefreshRoadMessage()
{
	RoadMessage = Chart ? Chart->RoadEndError(RoadFrom, RoadTo) : FString();
}

void FSolidWorldChartEdMode::Note(const FString& Message)
{
	ToolMessage = Message;
}

TArray<FIntPoint> FSolidWorldChartEdMode::PreviewCells() const
{
	if (!bRectangle)
	{
		return StrokeCells;
	}
	TArray<FIntPoint> Cells;
	const int32 X0 = FMath::Min(Anchor.X, CurrentCell.X);
	const int32 X1 = FMath::Max(Anchor.X, CurrentCell.X);
	const int32 Y0 = FMath::Min(Anchor.Y, CurrentCell.Y);
	const int32 Y1 = FMath::Max(Anchor.Y, CurrentCell.Y);
	for (int32 Y = Y0; Y <= Y1; ++Y)
	{
		for (int32 X = X0; X <= X1; ++X)
		{
			Cells.Add(FIntPoint(X, Y));
		}
	}
	return Cells;
}

FString FSolidWorldChartEdMode::DescribeCell(const FIntPoint& Cell) const
{
	if (!Chart)
	{
		return FString();
	}
	const int32 Index = Cell.Y * USolidWorldMap::MapWidth + Cell.X;
	if (!Chart->Cells.IsValidIndex(Index))
	{
		return FString();
	}

	const ESolidBiome Painted = Chart->Cells[Index];
	ESolidBiome Resolved = Painted;
	const USolidWorldMap* WorldMap = Streamer && Streamer->GetTerrainMap()
		? Streamer->GetTerrainMap()->GetWorldMap()
		: nullptr;
	if (WorldMap && WorldMap->IsLoaded())
	{
		Resolved = WorldMap->GetBiomeAtCell(Cell.X, Cell.Y);
	}

	FString Line = FString::Printf(TEXT("%s."), BiomeName(Resolved));
	if (Resolved != Painted)
	{
		Line += FString::Printf(TEXT(" Painted %s."), BiomeName(Painted));
	}

	const int32 MarkIndex = Chart->MarkAtCell(Cell);
	if (MarkIndex != INDEX_NONE)
	{
		const FSolidWorldChartMark& Mark = Chart->Marks[MarkIndex];
		Line += FString::Printf(TEXT(" %s, %s."), MarkKindName(Mark.Kind), *Mark.Name);
	}

	if (WorldMap)
	{
		const TArray<SolidRoad::FRoute>& Routes = WorldMap->GetRoads();
		for (int32 RoadIndex = 0; RoadIndex < Routes.Num(); ++RoadIndex)
		{
			if (!Routes[RoadIndex].Cells.Contains(Cell) || !Chart->Roads.IsValidIndex(RoadIndex))
			{
				continue;
			}
			const FSolidWorldChartRoad& Road = Chart->Roads[RoadIndex];
			Line += FString::Printf(
				TEXT(" Road %s -> %s, %s."),
				*Road.FromName,
				*Road.ToName,
				USolidWorldChart::RoadTypeName(Road.Type));
		}
	}
	return Line;
}
