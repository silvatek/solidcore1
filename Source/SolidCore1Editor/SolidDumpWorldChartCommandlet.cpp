#include "SolidDumpWorldChartCommandlet.h"

#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Strongholds/SolidStronghold.h"
#include "Terrain/SolidBiome.h"
#include "Terrain/SolidTerrainMap.h"
#include "Terrain/SolidWorldChart.h"

DEFINE_LOG_CATEGORY_STATIC(LogSolidChartDump, Log, All);

namespace
{
	const TCHAR* MarkKindName(const ESolidWorldChartMarkKind Kind)
	{
		switch (Kind)
		{
		case ESolidWorldChartMarkKind::Stronghold: return TEXT("Stronghold");
		case ESolidWorldChartMarkKind::StrongholdSite: return TEXT("Stronghold site");
		case ESolidWorldChartMarkKind::LairEntrance: return TEXT("Lair entrance");
		case ESolidWorldChartMarkKind::LairExit: return TEXT("Lair exit");
		case ESolidWorldChartMarkKind::Custom: return TEXT("Custom");
		case ESolidWorldChartMarkKind::Encounter: return TEXT("Encounter");
		case ESolidWorldChartMarkKind::Town:
		default:
			return TEXT("Town");
		}
	}

	/**
	 * Row and column count from 1 at the top-left, as Content/WorldMap.txt does.
	 * The position is a cell centre in the chart's centimetre rectangle.
	 */
	bool CellOfPosition(const FVector2D& PositionCm, int32& OutRow, int32& OutColumn)
	{
		FVector2D WorldMin = FVector2D::ZeroVector;
		FVector2D WorldMax = FVector2D::ZeroVector;
		USolidWorldChart::DefaultWorldBounds(WorldMin, WorldMax);
		const float ExtentX = FMath::Max(WorldMax.X - WorldMin.X, 1.f);
		const float ExtentY = FMath::Max(WorldMax.Y - WorldMin.Y, 1.f);
		const float CellW = ExtentX / static_cast<float>(USolidWorldChart::MapWidth);
		const float CellH = ExtentY / static_cast<float>(USolidWorldChart::MapHeight);
		const float ColumnIndex = (WorldMax.X - PositionCm.X) / CellW - 0.5f;
		const float RowIndex = (WorldMax.Y - PositionCm.Y) / CellH - 0.5f;
		const int32 Column = FMath::RoundToInt(ColumnIndex);
		const int32 Row = FMath::RoundToInt(RowIndex);
		if (Column < 0 || Row < 0
			|| Column >= USolidWorldChart::MapWidth || Row >= USolidWorldChart::MapHeight
			|| FMath::Abs(ColumnIndex - static_cast<float>(Column)) > 0.01f
			|| FMath::Abs(RowIndex - static_cast<float>(Row)) > 0.01f)
		{
			return false;
		}
		OutRow = Row + 1;
		OutColumn = Column + 1;
		return true;
	}

	/**
	 * Where the chart mode draws this mark. The chart rectangle starts at the
	 * terrain class default, and a built map is centred on the origin.
	 */
	FVector2D GroundCm(const FVector2D& PositionCm)
	{
		const USolidTerrainMap* Terrain = GetDefault<USolidTerrainMap>();
		const float Spacing = Terrain ? Terrain->PointSpacing : 200.f;
		const int32 Width = Terrain ? Terrain->GridWidth : 513;
		const int32 Height = Terrain ? Terrain->GridHeight : 513;
		const FVector2D BuiltMin(
			-0.5f * static_cast<float>(FMath::Max(Width - 1, 0)) * Spacing,
			-0.5f * static_cast<float>(FMath::Max(Height - 1, 0)) * Spacing);
		return USolidWorldChart::TerrainPosition(PositionCm, BuiltMin);
	}

	TSharedRef<FJsonObject> MarkJson(const FSolidWorldChartMark& Mark)
	{
		TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
		Object->SetStringField(TEXT("kind"), MarkKindName(Mark.Kind));
		Object->SetStringField(TEXT("name"), Mark.Name);
		int32 Row = 0;
		int32 Column = 0;
		if (CellOfPosition(Mark.PositionCm, Row, Column))
		{
			Object->SetNumberField(TEXT("row"), Row);
			Object->SetNumberField(TEXT("column"), Column);
		}
		TArray<TSharedPtr<FJsonValue>> Position;
		Position.Add(MakeShared<FJsonValueNumber>(Mark.PositionCm.X));
		Position.Add(MakeShared<FJsonValueNumber>(Mark.PositionCm.Y));
		Object->SetArrayField(TEXT("positionCm"), Position);
		const FVector2D Ground = GroundCm(Mark.PositionCm);
		TArray<TSharedPtr<FJsonValue>> GroundPosition;
		GroundPosition.Add(MakeShared<FJsonValueNumber>(Ground.X));
		GroundPosition.Add(MakeShared<FJsonValueNumber>(Ground.Y));
		Object->SetArrayField(TEXT("groundCm"), GroundPosition);
		Object->SetStringField(TEXT("strongholdSize"), SolidStronghold::SizeName(Mark.StrongholdSize));
		Object->SetStringField(TEXT("strongholdType"), SolidStronghold::TypeName(Mark.StrongholdType));
		Object->SetStringField(TEXT("lairId"), Mark.LairId);
		return Object;
	}
}

USolidDumpWorldChartCommandlet::USolidDumpWorldChartCommandlet()
{
	IsClient = false;
	IsEditor = true;
	LogToConsole = true;
	ShowErrorCount = true;
}

int32 USolidDumpWorldChartCommandlet::Main(const FString& Params)
{
	FString ChartPath = USolidWorldChart::ShippedPath();
	FString ChartArg;
	if (FParse::Value(*Params, TEXT("chart="), ChartArg))
	{
		ChartPath = ChartArg;
	}

	FString OutPath = FPaths::ProjectSavedDir() / TEXT("WorldChart.json");
	FString OutArg;
	if (FParse::Value(*Params, TEXT("out="), OutArg))
	{
		OutPath = OutArg;
	}
	OutPath = FPaths::ConvertRelativePathToFull(OutPath);

	USolidWorldChart* Chart = LoadObject<USolidWorldChart>(nullptr, *ChartPath);
	if (!Chart)
	{
		UE_LOG(LogSolidChartDump, Error, TEXT("World chart dump: failed to load %s."), *ChartPath);
		return 1;
	}

	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("asset"), Chart->GetPathName());
	Root->SetNumberField(TEXT("width"), USolidWorldChart::MapWidth);
	Root->SetNumberField(TEXT("height"), USolidWorldChart::MapHeight);
	Root->SetStringField(
		TEXT("origin"),
		TEXT("row 1, column 1 is the top-left, as in Content/WorldMap.txt. Column 0 of a row is east."));

	TSharedRef<FJsonObject> Key = MakeShared<FJsonObject>();
	Key->SetStringField(TEXT("."), TEXT("Grassland"));
	Key->SetStringField(TEXT("M"), TEXT("Mountain"));
	Key->SetStringField(TEXT("o"), TEXT("Town"));
	Key->SetStringField(TEXT("#"), TEXT("Forest"));
	Key->SetStringField(TEXT("^"), TEXT("Desert"));
	Key->SetStringField(TEXT("~"), TEXT("Sea"));
	Key->SetStringField(TEXT("*"), TEXT("River"));
	Key->SetStringField(TEXT("S"), TEXT("Swamp"));
	Key->SetStringField(TEXT("R"), TEXT("Road"));
	Root->SetObjectField(TEXT("key"), Key);

	TArray<TSharedPtr<FJsonValue>> Rows;
	Rows.Reserve(USolidWorldChart::MapHeight);
	if (Chart->Cells.Num() == USolidWorldChart::MapWidth * USolidWorldChart::MapHeight)
	{
		for (int32 Row = 0; Row < USolidWorldChart::MapHeight; ++Row)
		{
			FString Line;
			Line.Reserve(USolidWorldChart::MapWidth);
			for (int32 Column = 0; Column < USolidWorldChart::MapWidth; ++Column)
			{
				Line.AppendChar(SolidBiome::Of(Chart->Cells[Row * USolidWorldChart::MapWidth + Column]).Marker());
			}
			Rows.Add(MakeShared<FJsonValueString>(Line));
		}
	}
	else
	{
		UE_LOG(LogSolidChartDump, Warning,
			TEXT("World chart dump: the chart has %d cells, not %d."),
			Chart->Cells.Num(),
			USolidWorldChart::MapWidth * USolidWorldChart::MapHeight);
	}
	Root->SetArrayField(TEXT("rows"), Rows);

	// Terrain points. The first row is the north edge. The first character of a
	// row is the west edge. The letters are the same key as the cells.
	TArray<TSharedPtr<FJsonValue>> PointRows;
	PointRows.Reserve(USolidWorldChart::PointAxis);
	for (int32 North = USolidWorldChart::PointAxis - 1; North >= 0; --North)
	{
		FString Line;
		Line.Reserve(USolidWorldChart::PointAxis);
		for (int32 East = 0; East < USolidWorldChart::PointAxis; ++East)
		{
			const ESolidBiome Biome = Chart->GetPointBiome(FIntPoint(East, North));
			Line.AppendChar(SolidBiome::Of(Biome).Marker());
		}
		PointRows.Add(MakeShared<FJsonValueString>(Line));
	}
	Root->SetNumberField(TEXT("pointAxis"), USolidWorldChart::PointAxis);
	Root->SetStringField(
		TEXT("pointsOrigin"),
		TEXT("The first row is the north edge. The first character of a row is the west edge."));
	Root->SetArrayField(TEXT("points"), PointRows);

	TArray<TSharedPtr<FJsonValue>> Marks;
	Marks.Reserve(Chart->Marks.Num());
	for (const FSolidWorldChartMark& Mark : Chart->Marks)
	{
		Marks.Add(MakeShared<FJsonValueObject>(MarkJson(Mark)));
		int32 Row = 0;
		int32 Column = 0;
		const bool bCell = CellOfPosition(Mark.PositionCm, Row, Column);
		const FVector2D Ground = GroundCm(Mark.PositionCm);
		UE_LOG(LogSolidChartDump, Warning,
			TEXT("Mark %s \"%s\" row %s column %s position (%.0f, %.0f) ground (%.0f, %.0f)"),
			MarkKindName(Mark.Kind),
			*Mark.Name,
			bCell ? *FString::FromInt(Row) : TEXT("?"),
			bCell ? *FString::FromInt(Column) : TEXT("?"),
			Mark.PositionCm.X,
			Mark.PositionCm.Y,
			Ground.X,
			Ground.Y);
	}
	Root->SetArrayField(TEXT("marks"), Marks);

	TArray<TSharedPtr<FJsonValue>> Roads;
	Roads.Reserve(Chart->Roads.Num());
	for (const FSolidWorldChartRoad& Road : Chart->Roads)
	{
		TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
		Object->SetStringField(TEXT("from"), Road.FromName);
		Object->SetStringField(TEXT("to"), Road.ToName);
		Object->SetStringField(TEXT("type"), USolidWorldChart::RoadTypeName(Road.Type));
		Roads.Add(MakeShared<FJsonValueObject>(Object));
	}
	Root->SetArrayField(TEXT("roads"), Roads);

	FString Json;
	const TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer =
		TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&Json);
	if (!FJsonSerializer::Serialize(Root, Writer))
	{
		UE_LOG(LogSolidChartDump, Error, TEXT("World chart dump: JSON serialise failed."));
		return 1;
	}

	if (!FFileHelper::SaveStringToFile(Json, *OutPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		UE_LOG(LogSolidChartDump, Error, TEXT("World chart dump: failed to write %s."), *OutPath);
		return 1;
	}

	UE_LOG(LogSolidChartDump, Warning,
		TEXT("World chart dump: %d marks, %d roads, %d point rows, %s"),
		Chart->Marks.Num(),
		Chart->Roads.Num(),
		PointRows.Num(),
		*OutPath);
	return 0;
}
