#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidDungeonEntrance.h"
#include "SolidGameInstance.h"
#include "SolidGameState.h"
#include "SolidStronghold.h"
#include "SolidStrongholds.h"
#include "SolidWorldMap.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	FString MountainGrid()
	{
		FString Text;
		Text.Reserve((USolidWorldMap::MapWidth + 1) * USolidWorldMap::MapHeight);
		for (int32 Row = 0; Row < USolidWorldMap::MapHeight; ++Row)
		{
			for (int32 Col = 0; Col < USolidWorldMap::MapWidth; ++Col)
			{
				Text.AppendChar(TEXT('M'));
			}
			Text.AppendChar(TEXT('\n'));
		}
		return Text;
	}

	void SetCell(FString& Grid, const int32 Col, const int32 Row, const TCHAR Marker)
	{
		Grid[Row * (USolidWorldMap::MapWidth + 1) + Col] = Marker;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidStrongholdCatalogTest,
	"SolidCore1.Stronghold.Catalog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidStrongholdCatalogTest::RunTest(const FString& Parameters)
{
	const TCHAR* TypeNames[] = { TEXT("Dungeon"), TEXT("Fortress"), TEXT("Tower") };
	TestEqual(TEXT("three types"), SolidStronghold::TypeCount, 3);
	for (int32 Index = 0; Index < SolidStronghold::TypeCount; ++Index)
	{
		TestEqual(
			*FString::Printf(TEXT("type %d"), Index),
			FString(SolidStronghold::TypeName(SolidStronghold::Types[Index])),
			FString(TypeNames[Index]));
	}

	const TCHAR* SizeNames[] = {
		TEXT("Meagre"), TEXT("Humble"), TEXT("Average"), TEXT("Impressive"),
		TEXT("Grand"), TEXT("Stately"), TEXT("Imperious") };
	TestEqual(TEXT("seven sizes"), SolidStronghold::SizeCount, 7);
	for (int32 Index = 0; Index < SolidStronghold::SizeCount; ++Index)
	{
		TestEqual(
			*FString::Printf(TEXT("size %d"), Index),
			FString(SolidStronghold::SizeName(SolidStronghold::Sizes[Index])),
			FString(SizeNames[Index]));
	}

	const FSolidStronghold MeagreDungeon(
		ESolidStrongholdType::Dungeon,
		ESolidStrongholdSize::Meagre,
		SolidStronghold::StartingCell());
	TestEqual(
		TEXT("starting type"),
		static_cast<uint8>(MeagreDungeon.Type),
		static_cast<uint8>(ESolidStrongholdType::Dungeon));
	TestEqual(
		TEXT("starting size"),
		static_cast<uint8>(MeagreDungeon.Size),
		static_cast<uint8>(ESolidStrongholdSize::Meagre));
	TestEqual(
		TEXT("starting label"),
		SolidStronghold::Label(MeagreDungeon),
		FString(TEXT("Meagre Dungeon")));

	const SolidStronghold::FVisitView Cave = SolidStronghold::VisitViewFor(MeagreDungeon);
	TestTrue(TEXT("entering the cave levels the camera"), FMath::IsNearlyZero(Cave.Enter.PitchDeg, 0.1f));
	TestTrue(TEXT("entering the cave looks in"), FMath::IsNearlyZero(Cave.Enter.YawDeg, 0.1f));
	TestTrue(TEXT("leaving the cave levels the camera"), FMath::IsNearlyZero(Cave.Leave.PitchDeg, 0.1f));
	TestTrue(TEXT("leaving the cave looks out"), FMath::IsNearlyZero(Cave.Leave.YawDeg, 0.1f));
	TestTrue(
		TEXT("a north-facing mouth leaves looking north"),
		FMath::IsNearlyEqual(SolidStronghold::LeaveYawDeg(90.f, Cave.Leave), 90.f, 0.1f));

	FSolidStronghold Fortress;
	Fortress.Type = ESolidStrongholdType::Fortress;
	Fortress.Size = ESolidStrongholdSize::Meagre;
	const SolidStronghold::FVisitView Unlisted = SolidStronghold::VisitViewFor(Fortress);
	TestTrue(TEXT("an unlisted stronghold still arrives level"), FMath::IsNearlyZero(Unlisted.Enter.PitchDeg, 0.1f));
	TestTrue(TEXT("an unlisted stronghold still leaves level"), FMath::IsNearlyZero(Unlisted.Leave.PitchDeg, 0.1f));

	ESolidStrongholdType TokenType = ESolidStrongholdType::Fortress;
	ESolidStrongholdSize TokenSize = ESolidStrongholdSize::Imperious;
	TestTrue(TEXT("MeagreDungeon resolves"), SolidStronghold::FromToken(TEXT("MeagreDungeon"), TokenType, TokenSize));
	TestEqual(TEXT("the token is a Dungeon"), static_cast<uint8>(TokenType), static_cast<uint8>(ESolidStrongholdType::Dungeon));
	TestEqual(TEXT("the token is Meagre"), static_cast<uint8>(TokenSize), static_cast<uint8>(ESolidStrongholdSize::Meagre));
	TestFalse(TEXT("M does not resolve"), SolidStronghold::FromToken(TEXT("M"), TokenType, TokenSize));

	ESolidBiome Shelf = ESolidBiome::Grassland;
	TestTrue(TEXT("the cave names a shelf"), SolidStronghold::BiomeFor(MeagreDungeon, Shelf));
	TestEqual(TEXT("the cave shelf is mountain"), static_cast<uint8>(Shelf), static_cast<uint8>(ESolidBiome::Mountain));
	TestTrue(TEXT("the cave uses the cliff mouth"), SolidStronghold::UsesCaveMouth(MeagreDungeon));

	TestTrue(TEXT("ImperiousTower resolves"), SolidStronghold::FromToken(TEXT("ImperiousTower"), TokenType, TokenSize));
	const FSolidStronghold Imperious(TokenType, TokenSize);
	TestEqual(TEXT("Imperious Tower type"), static_cast<uint8>(Imperious.Type), static_cast<uint8>(ESolidStrongholdType::Tower));
	TestEqual(TEXT("Imperious Tower size"), static_cast<uint8>(Imperious.Size), static_cast<uint8>(ESolidStrongholdSize::Imperious));
	TestTrue(TEXT("the tower names a shelf"), SolidStronghold::BiomeFor(Imperious, Shelf));
	TestEqual(TEXT("the tower shelf is grassland"), static_cast<uint8>(Shelf), static_cast<uint8>(ESolidBiome::Grassland));
	TestFalse(TEXT("the tower does not use the cliff mouth"), SolidStronghold::UsesCaveMouth(Imperious));

	TestTrue(TEXT("Humble Tower resolves"), SolidStronghold::FromToken(TEXT("Humble Tower"), TokenType, TokenSize));
	const FSolidStronghold HumbleTower(TokenType, TokenSize);
	TestFalse(TEXT("an unlisted stronghold names no shelf"), SolidStronghold::BiomeFor(HumbleTower, Shelf));
	TestEqual(
		TEXT("a failed shelf lookup leaves the previous biome"),
		static_cast<uint8>(Shelf),
		static_cast<uint8>(ESolidBiome::Grassland));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidStrongholdShippedEntranceTest,
	"SolidCore1.Stronghold.ShippedEntrance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidStrongholdShippedEntranceTest::RunTest(const FString& Parameters)
{
	USolidWorldMap* Map = NewObject<USolidWorldMap>();
	TestNotNull(TEXT("world map"), Map);
	TestTrue(TEXT("LoadDefault"), Map->LoadDefault());

	TArray<FIntPoint> CaveCells;
	for (int32 Row = 0; Row < USolidWorldMap::MapHeight; ++Row)
	{
		for (int32 Col = 0; Col < USolidWorldMap::MapWidth; ++Col)
		{
			ESolidStrongholdType NamedType = ESolidStrongholdType::Fortress;
			ESolidStrongholdSize NamedSize = ESolidStrongholdSize::Imperious;
			if (!Map->StrongholdAtCell(Col, Row, NamedType, NamedSize))
			{
				continue;
			}
			const FSolidStronghold Named(NamedType, NamedSize);
			if (Named.Type != ESolidStrongholdType::Dungeon || Named.Size != ESolidStrongholdSize::Meagre)
			{
				ESolidBiome Expected = ESolidBiome::Grassland;
				TestTrue(TEXT("a shipped stronghold names its shelf"), SolidStronghold::BiomeFor(Named, Expected));
				TestEqual(
					TEXT("a shipped stronghold uses its shelf"),
					static_cast<uint8>(Map->GetBiomeAtCell(Col, Row)),
					static_cast<uint8>(Expected));
				continue;
			}
			CaveCells.Add(FIntPoint(Col, Row));
			TestEqual(
				TEXT("cave cell is Mountain"),
				static_cast<uint8>(Map->GetBiomeAtCell(Col, Row)),
				static_cast<uint8>(ESolidBiome::Mountain));
			TestEqual(TEXT("cave is not a town"), Map->GetLocationIdAtCell(Col, Row), INDEX_NONE);
			const FIntPoint North(Col, Row - 1);
			TestEqual(
				TEXT("grass marker north of the cave"),
				static_cast<int32>(Map->GetMarkerAtCell(North.X, North.Y)),
				static_cast<int32>(USolidWorldMap::GrassMarker));
			TestEqual(
				TEXT("north neighbor is grassland"),
				static_cast<uint8>(Map->GetBiomeAtCell(North.X, North.Y)),
				static_cast<uint8>(ESolidBiome::Grassland));
		}
	}
	TestTrue(TEXT("the shipped map has a cave"), CaveCells.Num() > 0);

	const FVector2D WorldMin(-51200.f, -51200.f);
	const FVector2D WorldMax(51200.f, 51200.f);
	int32 CaveMouths = 0;
	for (const FIntPoint& CaveCell : CaveCells)
	{
		SolidStrongholds::FEntrance Entrance;
		TestTrue(
			TEXT("the cave cell has a grass face"),
			SolidStrongholds::MakeEntrance(Map, WorldMin, WorldMax, CaveCell, Entrance));
		++CaveMouths;
		TestEqual(TEXT("entrance cell"), Entrance.Cell, CaveCell);
		const FIntPoint North(Entrance.Cell.X, Entrance.Cell.Y - 1);
		TestEqual(TEXT("grass column"), Entrance.GrassCell.X, North.X);
		TestEqual(TEXT("grass row"), Entrance.GrassCell.Y, North.Y);
		TestTrue(TEXT("yaw faces north"), FMath::IsNearlyEqual(Entrance.FacingYawDeg, 90.f, 0.1f));

		FVector2D CaveXY = FVector2D::ZeroVector;
		FVector2D GrassXY = FVector2D::ZeroVector;
		TestTrue(TEXT("cave projects"), Map->CellToWorldXY(Entrance.Cell, WorldMin, WorldMax, CaveXY));
		TestTrue(TEXT("grass projects"), Map->CellToWorldXY(North, WorldMin, WorldMax, GrassXY));
		const FVector2D Edge = (CaveXY + GrassXY) * 0.5f;
		TestTrue(TEXT("edge is the shared boundary"), Entrance.EdgeXY.Equals(Edge, 0.1f));
		TestTrue(TEXT("spawn is north of the edge"), Entrance.WorldXY.Y > Entrance.EdgeXY.Y);
		TestTrue(
			TEXT("spawn inset matches the grass-side offset"),
			FMath::IsNearlyEqual(FVector2D::Distance(Entrance.WorldXY, Entrance.EdgeXY), SolidStrongholds::FaceInsetCm, 0.1f));
	}
	TestEqual(TEXT("one cliff mouth per cave cell"), CaveMouths, CaveCells.Num());

	SolidStrongholds::FEntrance Nowhere;
	TestFalse(TEXT("null map yields none"), SolidStrongholds::MakeEntrance(nullptr, WorldMin, WorldMax, FIntPoint(0, 0), Nowhere));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidStrongholdFaceChoiceTest,
	"SolidCore1.Stronghold.FaceChoice",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidStrongholdFaceChoiceTest::RunTest(const FString& Parameters)
{
	const FVector2D WorldMin(0.f, 0.f);
	const FVector2D WorldMax(6400.f, 6400.f);

	{
		FString Grid = MountainGrid();
		SetCell(Grid, 10, 10, USolidWorldMap::LocationMarker);
		SetCell(Grid, 9, 10, USolidWorldMap::GrassMarker);
		Grid += TEXT("! 11,11 = Stronghold (MeagreDungeon)\n");
		USolidWorldMap* Map = NewObject<USolidWorldMap>();
		TestTrue(TEXT("east grass loads"), Map->LoadFromString(Grid));
		TestEqual(
			TEXT("a stronghold is Mountain"),
			static_cast<uint8>(Map->GetBiomeAtCell(10, 10)),
			static_cast<uint8>(ESolidBiome::Mountain));

		SolidStrongholds::FEntrance Entrance;
		TestTrue(TEXT("east face"), SolidStrongholds::MakeEntrance(Map, WorldMin, WorldMax, FIntPoint(10, 10), Entrance));
		TestEqual(TEXT("grass column is east"), Entrance.GrassCell.X, 9);
		TestEqual(TEXT("grass row"), Entrance.GrassCell.Y, 10);
		TestTrue(TEXT("yaw faces east"), FMath::IsNearlyEqual(Entrance.FacingYawDeg, 0.f, 0.1f));
		TestTrue(TEXT("spawn is east of the edge"), Entrance.WorldXY.X > Entrance.EdgeXY.X);
	}

	{
		FString Grid = MountainGrid();
		SetCell(Grid, 10, 10, TEXT('Q'));
		USolidWorldMap* Map = NewObject<USolidWorldMap>();
		TestTrue(TEXT("unknown letter loads"), Map->LoadFromString(Grid));
		TestEqual(
			TEXT("Q is still grassland"),
			static_cast<uint8>(Map->GetBiomeAtCell(10, 10)),
			static_cast<uint8>(ESolidBiome::Grassland));
	}

	{
		FString Grid = MountainGrid();
		SetCell(Grid, 10, 10, USolidWorldMap::LocationMarker);
		Grid += TEXT("! 11,11 = Stronghold (MeagreDungeon)\n");
		USolidWorldMap* Map = NewObject<USolidWorldMap>();
		TestTrue(TEXT("sealed cave loads"), Map->LoadFromString(Grid));
		SolidStrongholds::FEntrance Sealed;
		TestFalse(TEXT("no grass face means no mouth"), SolidStrongholds::MakeEntrance(Map, WorldMin, WorldMax, FIntPoint(10, 10), Sealed));
	}

	{
		FString Grid = MountainGrid();
		SetCell(Grid, 10, 10, USolidWorldMap::LocationMarker);
		SetCell(Grid, 10, 9, USolidWorldMap::GrassMarker);
		SetCell(Grid, 9, 10, USolidWorldMap::GrassMarker);
		Grid += TEXT("! 11,11 = Stronghold (MeagreDungeon)\n");
		USolidWorldMap* Map = NewObject<USolidWorldMap>();
		TestTrue(TEXT("two grass faces load"), Map->LoadFromString(Grid));
		SolidStrongholds::FEntrance Entrance;
		TestTrue(TEXT("one mouth"), SolidStrongholds::MakeEntrance(Map, WorldMin, WorldMax, FIntPoint(10, 10), Entrance));
		TestEqual(TEXT("north column wins"), Entrance.GrassCell.X, 10);
		TestEqual(TEXT("north row wins"), Entrance.GrassCell.Y, 9);
		TestTrue(TEXT("yaw faces north"), FMath::IsNearlyEqual(Entrance.FacingYawDeg, 90.f, 0.1f));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidDungeonEntranceVisualsTest,
	"SolidCore1.Stronghold.DungeonEntranceVisuals",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidDungeonEntranceVisualsTest::RunTest(const FString& Parameters)
{
	ASolidDungeonEntrance* Entrance = NewObject<ASolidDungeonEntrance>();
	TestNotNull(TEXT("entrance"), Entrance);
	Entrance->BuildVisuals();

	UStaticMeshComponent* Mouth = Entrance->GetMouthMesh();
	UStaticMeshComponent* Left = Entrance->GetLeftJamb();
	UStaticMeshComponent* Right = Entrance->GetRightJamb();
	UStaticMeshComponent* Lintel = Entrance->GetLintel();
	TestNotNull(TEXT("mouth"), Mouth);
	TestNotNull(TEXT("left jamb"), Left);
	TestNotNull(TEXT("right jamb"), Right);
	TestNotNull(TEXT("lintel"), Lintel);
	if (!Mouth || !Left || !Right || !Lintel)
	{
		return false;
	}

	UStaticMesh* MouthMesh = Mouth->GetStaticMesh();
	UStaticMesh* LeftMesh = Left->GetStaticMesh();
	UStaticMesh* LintelMesh = Lintel->GetStaticMesh();
	TestNotNull(TEXT("mouth cube"), MouthMesh);
	TestNotNull(TEXT("jamb cube"), LeftMesh);
	TestNotNull(TEXT("lintel cube"), LintelMesh);

	TestTrue(TEXT("mouth is wider than it is deep"), Mouth->GetRelativeScale3D().Y > Mouth->GetRelativeScale3D().X);
	TestTrue(TEXT("jambs are thinner than the mouth"), Left->GetRelativeScale3D().Y < Mouth->GetRelativeScale3D().Y);
	TestTrue(TEXT("left jamb is on -Y"), Left->GetRelativeLocation().Y < 0.f);
	TestTrue(TEXT("right jamb is on +Y"), Right->GetRelativeLocation().Y > 0.f);
	TestTrue(TEXT("lintel sits above the mouth"), Lintel->GetRelativeLocation().Z > Mouth->GetRelativeLocation().Z);
	TestTrue(TEXT("mouth meets the ground"), FMath::IsNearlyEqual(
		Mouth->GetRelativeLocation().Z - Mouth->GetRelativeScale3D().Z * 50.f, 0.f, 1.f));
	const float MouthFront = Mouth->GetRelativeLocation().X + Mouth->GetRelativeScale3D().X * 50.f;
	const float LintelFront = Lintel->GetRelativeLocation().X + Lintel->GetRelativeScale3D().X * 50.f;
	TestTrue(TEXT("mouth is recessed behind the lintel front"), MouthFront < LintelFront - 1.f);
	TestTrue(TEXT("the grass face of the frame sits toward the cliff"),
		FMath::IsNearlyEqual(LintelFront, SolidStrongholds::FrameFrontLocalX, 1.f));
	TestTrue(TEXT("the grass spawn stands in front of the mouth"), MouthFront < -40.f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidStrongholdSizeTokenTest,
	"SolidCore1.Stronghold->SizeToken",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidStrongholdSizeTokenTest::RunTest(const FString& Parameters)
{
	ESolidStrongholdSize Size = ESolidStrongholdSize::Imperious;
	TestTrue(TEXT("Humble is a size"), SolidStronghold::SizeFromToken(TEXT("Humble"), Size));
	TestEqual(TEXT("Humble size"), static_cast<uint8>(Size), static_cast<uint8>(ESolidStrongholdSize::Humble));
	TestTrue(TEXT("a lower-case size matches"), SolidStronghold::SizeFromToken(TEXT("humble"), Size));
	TestEqual(TEXT("lower-case size"), static_cast<uint8>(Size), static_cast<uint8>(ESolidStrongholdSize::Humble));
	TestTrue(TEXT("spaces around a size are ignored"), SolidStronghold::SizeFromToken(TEXT("  Average  "), Size));
	TestEqual(TEXT("trimmed size"), static_cast<uint8>(Size), static_cast<uint8>(ESolidStrongholdSize::Average));

	ESolidStrongholdType JoinedType = ESolidStrongholdType::Dungeon;
	ESolidStrongholdSize JoinedSize = ESolidStrongholdSize::Meagre;
	TestFalse(TEXT("a size alone is not a stronghold"), SolidStronghold::FromToken(TEXT("Humble"), JoinedType, JoinedSize));
	TestFalse(TEXT("a size and type is not a size"), SolidStronghold::SizeFromToken(TEXT("Humble Tower"), Size));
	TestFalse(TEXT("a joined token is not a size"), SolidStronghold::SizeFromToken(TEXT("MeagreDungeon"), Size));
	TestFalse(TEXT("an empty token is not a size"), SolidStronghold::SizeFromToken(TEXT(""), Size));
	TestFalse(TEXT("an unknown word is not a size"), SolidStronghold::SizeFromToken(TEXT("Vast"), Size));
	TestEqual(
		TEXT("a failed size lookup leaves the previous size"),
		static_cast<uint8>(Size),
		static_cast<uint8>(ESolidStrongholdSize::Average));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidStrongholdActiveCompanyTest,
	"SolidCore1.Stronghold.ActiveCompany",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidStrongholdActiveCompanyTest::RunTest(const FString& Parameters)
{
	const FGameState Fresh;
	TestEqual(
		TEXT("a new game is a dungeon"),
		static_cast<uint8>(Fresh.ActiveStronghold->Type),
		static_cast<uint8>(ESolidStrongholdType::Dungeon));
	TestEqual(
		TEXT("a new game is meagre"),
		static_cast<uint8>(Fresh.ActiveStronghold->Size),
		static_cast<uint8>(ESolidStrongholdSize::Meagre));
	TestEqual(TEXT("a new game uses the starting column"), Fresh.ActiveStronghold->Cell.X, SolidStronghold::StartingColumn - 1);
	TestEqual(TEXT("a new game uses the starting row"), Fresh.ActiveStronghold->Cell.Y, SolidStronghold::StartingRow - 1);
	TestTrue(
		TEXT("a new game has reached Meagre"),
		SolidEvents::IsStrongholdSizeUnlocked(Fresh, ESolidStrongholdSize::Meagre));
	for (const ESolidStrongholdSize Size : SolidStronghold::Sizes)
	{
		if (Size == ESolidStrongholdSize::Meagre)
		{
			continue;
		}
		TestFalse(
			*FString::Printf(TEXT("%s starts locked"), SolidStronghold::SizeName(Size)),
			SolidEvents::IsStrongholdSizeUnlocked(Fresh, Size));
	}

	USolidWorldMap* Map = NewObject<USolidWorldMap>();
	TestNotNull(TEXT("world map"), Map);
	TestTrue(TEXT("shipped map loads"), Map && Map->LoadDefault());
	if (Map)
	{
		const FIntPoint Cell = SolidStronghold::StartingCell();
		FSolidWorldBang Cave;
		TestTrue(TEXT("the starting cell is described"), Map->FindBang(Cell.X, Cell.Y, Cave));
		TestEqual(TEXT("the starting cell is a stronghold"), static_cast<uint8>(Cave.Kind), static_cast<uint8>(ESolidWorldBang::Stronghold));
		TestEqual(TEXT("the starting cell is meagre"), static_cast<uint8>(Cave.StrongholdSize), static_cast<uint8>(ESolidStrongholdSize::Meagre));
		TestEqual(TEXT("the starting cell is a dungeon"), static_cast<uint8>(Cave.StrongholdType), static_cast<uint8>(ESolidStrongholdType::Dungeon));
		ESolidStrongholdSize SiteSize = ESolidStrongholdSize::Humble;
		TestFalse(TEXT("the starting cell is not a site"), Map->StrongholdSiteAtCell(Cell.X, Cell.Y, SiteSize));

		int32 ImperiousTowers = 0;
		for (int32 Row = 0; Row < USolidWorldMap::MapHeight; ++Row)
		{
			for (int32 Col = 0; Col < USolidWorldMap::MapWidth; ++Col)
			{
				ESolidStrongholdType NamedType = ESolidStrongholdType::Dungeon;
				ESolidStrongholdSize NamedSize = ESolidStrongholdSize::Meagre;
				if (!Map->StrongholdAtCell(Col, Row, NamedType, NamedSize))
				{
					continue;
				}
				if (NamedType == ESolidStrongholdType::Tower && NamedSize == ESolidStrongholdSize::Imperious)
				{
					++ImperiousTowers;
				}
			}
		}
		TestEqual(TEXT("the Imperious Tower visit is gone"), ImperiousTowers, 0);

		ESolidStrongholdSize ImperiousSite = ESolidStrongholdSize::Meagre;
		TestTrue(
			TEXT("the Imperious site stays"),
			Map->StrongholdSiteAtCell(35, 15, ImperiousSite));
		TestEqual(
			TEXT("that site is Imperious"),
			static_cast<uint8>(ImperiousSite),
			static_cast<uint8>(ESolidStrongholdSize::Imperious));

		SolidStrongholds::FEntrance SiteDoor;
		const FVector2D WorldMin(0.f, 0.f);
		const FVector2D WorldMax(102400.f, 102400.f);
		TestTrue(
			TEXT("a built site still has a door face"),
			SolidStrongholds::MakeEntrance(
				Map,
				WorldMin,
				WorldMax,
				FIntPoint(35, 15),
				SiteDoor));
		TestEqual(TEXT("that door uses the site cell"), SiteDoor.Cell, FIntPoint(35, 15));
	}

	FGameState Moved;
	Moved.ActiveStronghold = SolidStronghold::Make(
		ESolidStrongholdType::Tower,
		ESolidStrongholdSize::Average,
		FIntPoint(4, 6));
	Moved.MaxStrongholdSize = ESolidStrongholdSize::Average;

	USolidGameInstance* Instance = NewObject<USolidGameInstance>();
	TestNotNull(TEXT("game instance"), Instance);
	if (!Instance)
	{
		return false;
	}
	FSolidTravelSession Session;
	Session.Configuration = Moved;
	Instance->Capture(Session);
	const FGameState& Carried = Instance->GetSession().Configuration;
	TestEqual(
		TEXT("the trip keeps the type"),
		static_cast<uint8>(Carried.ActiveStronghold->Type),
		static_cast<uint8>(ESolidStrongholdType::Tower));
	TestEqual(
		TEXT("the trip keeps the size"),
		static_cast<uint8>(Carried.ActiveStronghold->Size),
		static_cast<uint8>(ESolidStrongholdSize::Average));
	TestEqual(TEXT("the trip keeps the column"), Carried.ActiveStronghold->Cell.X, 4);
	TestEqual(TEXT("the trip keeps the row"), Carried.ActiveStronghold->Cell.Y, 6);
	TestTrue(
		TEXT("the trip keeps the unlocked size"),
		SolidEvents::IsStrongholdSizeUnlocked(Carried, ESolidStrongholdSize::Average));
	TestTrue(
		TEXT("the trip keeps the sizes below that"),
		SolidEvents::IsStrongholdSizeUnlocked(Carried, ESolidStrongholdSize::Humble));
	TestFalse(
		TEXT("the trip does not unlock a larger size"),
		SolidEvents::IsStrongholdSizeUnlocked(Carried, ESolidStrongholdSize::Impressive));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
