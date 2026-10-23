#pragma once

#include "CoreMinimal.h"
#include "EdMode.h"
#include "Terrain/SolidWorldChart.h"

class ASolidTerrainStreamer;
class ATextRenderActor;
class FSolidWorldChartToolkit;
class SWidget;
class ULineBatchComponent;

/** What a left click does in the world chart. */
enum class ESolidWorldChartTool : uint8
{
	/** Left click does not edit. */
	View,
	/** Left click paints the brush biome. Shift drags a rectangle. */
	Paint,
	/** Left click places the current mark on that cell. */
	Place,
};

/**
 * World chart mode. Paint, marks, and roads edit /Game/World/WorldChart.
 * The ground is the existing terrain build, fed by that chart.
 */
class FSolidWorldChartEdMode : public FEdMode
{
public:
	static const FEditorModeID Id;

	FSolidWorldChartEdMode();
	virtual ~FSolidWorldChartEdMode() override;

	virtual void Enter() override;
	virtual void Exit() override;
	virtual void Tick(FEditorViewportClient* ViewportClient, float DeltaTime) override;
	virtual void Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI) override;
	virtual void DrawHUD(FEditorViewportClient* ViewportClient, FViewport* Viewport, const FSceneView* View, FCanvas* Canvas) override;
	virtual bool InputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event) override;
	virtual bool MouseMove(FEditorViewportClient* ViewportClient, FViewport* Viewport, int32 X, int32 Y) override;
	virtual bool CapturedMouseMove(FEditorViewportClient* ViewportClient, FViewport* Viewport, int32 X, int32 Y) override;
	virtual bool StartTracking(FEditorViewportClient* ViewportClient, FViewport* Viewport) override;
	virtual bool EndTracking(FEditorViewportClient* ViewportClient, FViewport* Viewport) override;
	virtual bool HandleClick(FEditorViewportClient* ViewportClient, HHitProxy* HitProxy, const FViewportClick& Click) override;
	virtual bool UsesToolkits() const override { return true; }
	virtual bool UsesTransformWidget() const override { return false; }
	virtual bool IsSelectionAllowed(AActor* InActor, bool bInSelection) const override { return false; }
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override { return TEXT("FSolidWorldChartEdMode"); }

	ESolidWorldChartTool Tool = ESolidWorldChartTool::View;
	ESolidBiome BrushBiome = ESolidBiome::Grassland;
	ESolidWorldChartMarkKind MarkKind = ESolidWorldChartMarkKind::Town;
	int32 TownIndex = 0;
	int32 SizeIndex = 0;
	int32 TypeIndex = 0;
	int32 LairIndex = 0;
	int32 EncounterIndex = 0;
	FString CustomNote;
	ESolidWorldChartRoadType RoadType = ESolidWorldChartRoadType::Trail;
	FString RoadFrom;
	FString RoadTo;

	void SetTool(ESolidWorldChartTool InTool);
	void SetBrush(ESolidBiome Biome);
	void RoadNamesChanged();
	void PlaceCurrentMark();
	void DeleteCurrentMark();
	void AddCurrentRoad();
	void UndoChart();
	void SaveChart();

	FString BannerText() const { return Banner; }
	FString HoverLine() const { return Hover; }
	FString RoadLine() const { return RoadMessage; }
	FString ToolLine() const { return ToolMessage; }
	void RefreshPanel();

	static const TCHAR* OpenWorldLevel;

private:

	TObjectPtr<USolidWorldChart> Chart;
	TObjectPtr<ASolidTerrainStreamer> Streamer;
	TObjectPtr<AActor> Overlay;
	TObjectPtr<ULineBatchComponent> Lines;
	TArray<TObjectPtr<ATextRenderActor>> Signs;
	TSharedPtr<FSolidWorldChartToolkit> ChartToolkit;

	bool bStroke = false;
	bool bRectangle = false;
	/** True after this left click has already placed a mark. */
	bool bPlacedClick = false;
	FIntPoint Anchor = FIntPoint::ZeroValue;
	FIntPoint CurrentCell = FIntPoint::ZeroValue;
	bool bHasCell = false;
	TArray<FIntPoint> StrokeCells;

	FString Banner;
	FString Hover;
	FString RoadMessage;
	FString ToolMessage;

	bool IsOpenWorld(const UWorld* World) const;
	bool CursorCell(FEditorViewportClient* ViewportClient, FViewport* Viewport, int32 X, int32 Y, FIntPoint& OutCell) const;
	void HoverAt(FEditorViewportClient* ViewportClient, FViewport* Viewport, int32 X, int32 Y);
	void PlaceUnderCursor(FEditorViewportClient* ViewportClient, FViewport* Viewport);
	void BeginStroke(FEditorViewportClient* ViewportClient, FViewport* Viewport);
	void ExtendStroke(FEditorViewportClient* ViewportClient, FViewport* Viewport, int32 X, int32 Y);
	void EndStroke();
	void SampleBrush(const FIntPoint& Cell);
	void ShowChart();
	void ClearChartActors();
	void RebuildOverlay();
	void RebuildSigns();
	void FaceSigns(const FVector& Camera);
	bool MarkLocation(const FSolidWorldChartMark& Mark, FVector& OutLocation) const;
	void RefreshRoadMessage();
	void Note(const FString& Message);
	TArray<FIntPoint> PreviewCells() const;
	FString DescribeCell(const FIntPoint& Cell) const;
};
