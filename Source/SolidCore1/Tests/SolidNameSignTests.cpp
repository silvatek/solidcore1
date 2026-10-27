#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Company/SolidCompany.h"
#include "Menus/SolidMainMenu.h"
#include "SolidGameInstance.h"
#include "SolidGameState.h"
#include "SolidHUD.h"
#include "SolidStronghold.h"
#include "SolidTravelSession.h"
#include "Strongholds/Dungeon/SolidDungeon.h"
#include "Strongholds/Dungeon/SolidDungeonLevel.h"
#include "Strongholds/Fortress/SolidFortress.h"
#include "Strongholds/SolidNameSign.h"
#include "Strongholds/SolidNameSigns.h"
#include "Strongholds/Tower/SolidTower.h"
#include "Components/StaticMeshComponent.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace SolidNameSignTest
{
	void RaiseMaxSize(USolidGameState& State, const ESolidStrongholdSize Size)
	{
		if (static_cast<int32>(Size) > static_cast<int32>(State.MaxStrongholdSize))
		{
			State.MaxStrongholdSize = Size;
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidNameSignDefaultTest,
	"SolidCore1.NameSign.Default",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidNameSignDefaultTest::RunTest(const FString& Parameters)
{
	USolidCompany* Company = NewObject<USolidCompany>();
	Company->InitializeStartingStronghold();
	const FSolidStronghold* Starting = Company->GetStronghold();
	TestNotNull(TEXT("a new game has a stronghold"), Starting);
	if (!Starting)
	{
		return false;
	}
	TestEqual(TEXT("the starting name is The Cave"), Starting->Name, FString(SolidStronghold::StartingStrongholdName));
	TestEqual(TEXT("the starting label is still Meagre Dungeon"), SolidStronghold::Label(*Starting), FString(TEXT("Meagre Dungeon")));
	TestEqual(TEXT("The Cave is the default there"), SolidStronghold::DefaultName(*Starting), Starting->Name);
	TestEqual(TEXT("the size is Meagre"), static_cast<uint8>(Starting->Size), static_cast<uint8>(ESolidStrongholdSize::Meagre));
	TestEqual(TEXT("the type is Dungeon"), static_cast<uint8>(Starting->Type), static_cast<uint8>(ESolidStrongholdType::Dungeon));

	const FIntPoint Elsewhere(3, 4);
	TUniquePtr<FSolidStronghold> OtherCave = SolidStronghold::Make(
		ESolidStrongholdType::Dungeon,
		ESolidStrongholdSize::Meagre,
		Elsewhere,
		SolidStronghold::WorldXY(Elsewhere));
	TestEqual(TEXT("another Meagre Dungeon is named from its size and type"), OtherCave->Name, FString(TEXT("Meagre Dungeon")));
	TestEqual(TEXT("that name is its default"), SolidStronghold::DefaultName(*OtherCave), OtherCave->Name);

	TUniquePtr<FSolidStronghold> UnnamedStart = SolidStronghold::Make(
		ESolidStrongholdType::Dungeon,
		ESolidStrongholdSize::Meagre,
		SolidStronghold::StartingCell(),
		SolidStronghold::WorldXY(SolidStronghold::StartingCell()));
	TestEqual(TEXT("Make does not write The Cave"), UnnamedStart->Name, FString(TEXT("Meagre Dungeon")));
	TestEqual(
		TEXT("the starting cell's Meagre Dungeon still defaults to The Cave"),
		SolidStronghold::DefaultName(*UnnamedStart),
		FString(SolidStronghold::StartingStrongholdName));

	const FIntPoint TowerCell(8, 9);
	TUniquePtr<FSolidStronghold> Tower = SolidStronghold::Make(
		ESolidStrongholdType::Tower,
		ESolidStrongholdSize::Average,
		TowerCell,
		SolidStronghold::WorldXY(TowerCell));
	TestEqual(TEXT("an Average Tower is named from its size and type"), Tower->Name, FString(TEXT("Average Tower")));
	TestEqual(TEXT("that tower's default is its label"), SolidStronghold::DefaultName(*Tower), Tower->Name);

	TestEqual(TEXT("the journal enter line stays"), FString(SolidStronghold::EnterLabel), FString(TEXT("Enter stronghold")));
	TestEqual(TEXT("the journal exit line stays"), FString(SolidStronghold::ExitLabel), FString(TEXT("Exit stronghold")));
	TestEqual(TEXT("the cave pointer tip stays"), FString(SolidDungeonLevel::EnterLabel), FString(TEXT("Enter cave")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidNameSignTypingTest,
	"SolidCore1.NameSign.Typing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidNameSignTypingTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("spaces around a name are dropped"), SolidNameSigns::Keep(TEXT("  High Keep  ")), FString(TEXT("High Keep")));
	TestEqual(TEXT("a blank line is not a name"), SolidNameSigns::Keep(TEXT("   ")), FString());
	FString Long;
	for (int32 Index = 0; Index < 30; ++Index)
	{
		Long.AppendChar(TEXT('a'));
	}
	TestEqual(TEXT("a kept name is at most 24 characters"), SolidNameSigns::Keep(Long).Len(), SolidNameSigns::MaxCharacters);
	TestEqual(TEXT("the extra letters are the ones dropped"), SolidNameSigns::Keep(Long), Long.Left(SolidNameSigns::MaxCharacters));

	FString Cut = Long.Left(23);
	Cut.AppendChar(TEXT(' '));
	Cut.AppendChar(TEXT('z'));
	TestEqual(TEXT("a space left by the cut is dropped"), SolidNameSigns::Keep(Cut), Long.Left(23));

	FString Line;
	TestTrue(TEXT("a letter is typed"), SolidNameSigns::ApplyKey(Line, EKeys::H, true));
	TestEqual(TEXT("shift writes the capital"), Line, FString(TEXT("H")));
	TestTrue(TEXT("a lower letter follows"), SolidNameSigns::ApplyKey(Line, EKeys::I, false));
	TestEqual(TEXT("the line is Hi"), Line, FString(TEXT("Hi")));
	TestTrue(TEXT("a space is typed"), SolidNameSigns::ApplyKey(Line, EKeys::SpaceBar, false));
	TestTrue(TEXT("a digit is typed"), SolidNameSigns::ApplyKey(Line, EKeys::Two, true));
	TestTrue(TEXT("a numpad digit is typed"), SolidNameSigns::ApplyKey(Line, EKeys::NumPadThree, false));
	TestEqual(TEXT("digits ignore shift"), Line, FString(TEXT("Hi 23")));
	TestFalse(TEXT("Enter is not an edit"), SolidNameSigns::ApplyKey(Line, EKeys::Enter, false));
	TestFalse(TEXT("Esc is not an edit"), SolidNameSigns::ApplyKey(Line, EKeys::Escape, false));
	TestEqual(TEXT("Enter and Esc leave the line"), Line, FString(TEXT("Hi 23")));
	TestTrue(TEXT("Backspace removes one character"), SolidNameSigns::ApplyKey(Line, EKeys::BackSpace, false));
	TestEqual(TEXT("the line is Hi 2"), Line, FString(TEXT("Hi 2")));

	FString Full;
	for (int32 Index = 0; Index < SolidNameSigns::MaxCharacters; ++Index)
	{
		TestTrue(TEXT("the line accepts 24 letters"), SolidNameSigns::ApplyKey(Full, EKeys::A, false));
	}
	TestFalse(TEXT("the 25th letter is dropped"), SolidNameSigns::ApplyKey(Full, EKeys::B, false));
	TestEqual(TEXT("the line stays 24"), Full.Len(), SolidNameSigns::MaxCharacters);
	FString Empty;
	TestFalse(TEXT("Backspace on an empty line changes nothing"), SolidNameSigns::ApplyKey(Empty, EKeys::BackSpace, false));
	TestTrue(TEXT("the empty line stays empty"), Empty.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidNameSignMoveTest,
	"SolidCore1.NameSign.Move",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidNameSignMoveTest::RunTest(const FString& Parameters)
{
	const FIntPoint ImperiousCell(35, 15);

	USolidCompany* Company = NewObject<USolidCompany>();
	Company->InitializeStartingStronghold();
	USolidGameState& State = *NewObject<USolidGameState>();
	State.bStrongholdSiteRestrictions = true;
	SolidNameSignTest::RaiseMaxSize(State, ESolidStrongholdSize::Imperious);
	TestTrue(
		TEXT("leaving The Cave names the tower from its size and type"),
		SolidStrongholdTransition::Move(
			*Company, State, ImperiousCell, ESolidStrongholdSize::Imperious, ESolidStrongholdType::Tower));
	TestEqual(TEXT("the new stronghold is Imperious Tower"), Company->GetStronghold()->Name, FString(TEXT("Imperious Tower")));
	TestEqual(
		TEXT("the cave keeps The Cave"),
		State.PreviousStrongholds[0]->Name,
		FString(SolidStronghold::StartingStrongholdName));

	USolidCompany* Named = NewObject<USolidCompany>();
	Named->InitializeStartingStronghold();
	Named->GetStronghold()->Name = TEXT("High Keep");
	USolidGameState& NamedState = *NewObject<USolidGameState>();
	NamedState.bStrongholdSiteRestrictions = true;
	SolidNameSignTest::RaiseMaxSize(NamedState, ESolidStrongholdSize::Imperious);
	TestTrue(
		TEXT("a chosen name moves with the company"),
		SolidStrongholdTransition::Move(
			*Named, NamedState, ImperiousCell, ESolidStrongholdSize::Imperious, ESolidStrongholdType::Tower));
	TestEqual(TEXT("the tower is High Keep"), Named->GetStronghold()->Name, FString(TEXT("High Keep")));
	TestEqual(TEXT("the cave that was left is High Keep"), NamedState.PreviousStrongholds[0]->Name, FString(TEXT("High Keep")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidNameSignTripTest,
	"SolidCore1.NameSign.Trip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidNameSignTripTest::RunTest(const FString& Parameters)
{
	USolidGameInstance* Instance = NewObject<USolidGameInstance>();
	TestNotNull(TEXT("game instance"), Instance);
	if (!Instance)
	{
		return false;
	}
	USolidCompany* Company = Instance->GetCompany();
	TestNotNull(TEXT("the company"), Company);
	if (!Company || !Company->GetStronghold())
	{
		return false;
	}
	Company->GetStronghold()->Name = TEXT("High Keep");
	Instance->Capture(FSolidTravelSession());
	TestTrue(TEXT("a trip keeps the company"), Instance->GetCompany() == Company);
	TestEqual(TEXT("a trip keeps the name"), Company->GetStronghold()->Name, FString(TEXT("High Keep")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidNameSignPlaceTest,
	"SolidCore1.NameSign.Place",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidNameSignPlaceTest::RunTest(const FString& Parameters)
{
	TUniquePtr<FSolidStronghold> Cave = SolidStronghold::Make(
		ESolidStrongholdType::Dungeon,
		ESolidStrongholdSize::Meagre,
		SolidStronghold::StartingCell(),
		FVector2D(1000.f, 2000.f));
	Cave->Orientation = ESolidStrongholdOrientation::North;
	Cave->Name = SolidStronghold::StartingStrongholdName;
	const SolidStronghold::FSolidDungeon& Dungeon = static_cast<const SolidStronghold::FSolidDungeon&>(*Cave);
	const FSolidNameSignPlace North = Dungeon.NameSign();
	const FVector NorthPole = SolidNameSigns::PoleLocalCm(North, Dungeon.Name);
	const float NorthOpening = Dungeon.ShelfLateralCm(ESolidStrongholdOrientation::North)
		+ SolidStrongholds::OpeningWidthCm * 0.5f;
	const float NorthNear = NorthPole.Y - SolidNameSigns::BoardWidthCm(Dungeon.Name) * 0.5f;
	TestTrue(
		TEXT("the cave board clears the mouth by the frame and a step"),
		NorthNear >= NorthOpening + SolidStrongholds::FrameThicknessCm + SolidNameSigns::StepCm - 0.1f);
	const FVector NorthStand(Dungeon.ExitReachCm(), Dungeon.ExitLateralCm(), 0.f);
	TestTrue(
		TEXT("the cave sign is clear of the captain's stand"),
		FVector::Dist2D(NorthStand, NorthPole) > Dungeon.LeaveStandCm());

	Cave->Orientation = ESolidStrongholdOrientation::East;
	const FSolidNameSignPlace East = Dungeon.NameSign();
	TestFalse(TEXT("east and north use different frame edges"), North.FrameEdgeCm.Equals(East.FrameEdgeCm, 1.f));
	TestTrue(TEXT("the cave board faces the grass"), FMath::IsNearlyEqual(North.YawDeg, 0.f));

	const SolidNameSigns::FWorldPlace World = SolidNameSigns::WorldPlace(*Cave, nullptr);
	Cave->Orientation = ESolidStrongholdOrientation::North;
	const FVector Local = SolidNameSigns::PoleLocalCm(Cave->NameSign(), Cave->Name);
	const FVector Expected = FTransform(
		FRotator(0.f, 90.f, 0.f),
		FVector(Cave->Location.X, Cave->Location.Y, 0.f)).TransformPosition(Local);
	const SolidNameSigns::FWorldPlace NorthWorld = SolidNameSigns::WorldPlace(*Cave, nullptr);
	TestTrue(TEXT("the pole stands in the exterior's world frame"), NorthWorld.XY.Equals(FVector2D(Expected.X, Expected.Y), 0.1f));
	TestTrue(TEXT("a north cave sign faces north"), FMath::IsNearlyEqual(FRotator::NormalizeAxis(NorthWorld.YawDeg), 90.f, 0.1f));
	TestTrue(TEXT("an east cave sign faces east"), FMath::IsNearlyEqual(FRotator::NormalizeAxis(World.YawDeg), 0.f, 0.1f));

	const FIntPoint TowerCell(8, 9);
	TUniquePtr<FSolidStronghold> Average = SolidStronghold::Make(
		ESolidStrongholdType::Tower,
		ESolidStrongholdSize::Average,
		TowerCell,
		SolidStronghold::WorldXY(TowerCell));
	TUniquePtr<FSolidStronghold> Imperious = SolidStronghold::Make(
		ESolidStrongholdType::Tower,
		ESolidStrongholdSize::Imperious,
		TowerCell,
		SolidStronghold::WorldXY(TowerCell));
	const SolidStronghold::FSolidTower& AverageTower = static_cast<const SolidStronghold::FSolidTower&>(*Average);
	const SolidStronghold::FSolidTower& ImperiousTower = static_cast<const SolidStronghold::FSolidTower&>(*Imperious);
	TestTrue(
		TEXT("a larger tower moves the door, and the sign with it"),
		FMath::Abs(AverageTower.NameSign().FrameEdgeCm.X - ImperiousTower.NameSign().FrameEdgeCm.X) > 1.f);
	TestTrue(TEXT("the tower board faces the grass"), FMath::IsNearlyEqual(AverageTower.NameSign().YawDeg, 180.f));
	const FVector TowerPole = SolidNameSigns::PoleLocalCm(AverageTower.NameSign(), AverageTower.Name);
	const float DoorEdge = AverageTower.Plan().DoorWidthCm * 0.5f;
	const float TowerNear = TowerPole.Y - SolidNameSigns::BoardWidthCm(AverageTower.Name) * 0.5f;
	TestTrue(
		TEXT("the tower board clears the door by a step"),
		TowerNear >= DoorEdge + SolidNameSigns::StepCm - 0.1f);
	const FVector TowerStand(-AverageTower.ExitReachCm(), 0.f, 0.f);
	TestTrue(
		TEXT("the tower sign is clear of the captain's stand"),
		FVector::Dist(TowerStand, TowerPole) > SolidStronghold::DoorStandClearCm);

	const FIntPoint FortCell(4, 5);
	TUniquePtr<FSolidStronghold> Humble = SolidStronghold::Make(
		ESolidStrongholdType::Fortress,
		ESolidStrongholdSize::Humble,
		FortCell,
		SolidStronghold::WorldXY(FortCell));
	const SolidStronghold::FSolidFortress& Fort = static_cast<const SolidStronghold::FSolidFortress&>(*Humble);
	TestTrue(TEXT("the fortress board faces the grass"), FMath::IsNearlyEqual(Fort.NameSign().YawDeg, 180.f));
	const FVector FortPole = SolidNameSigns::PoleLocalCm(Fort.NameSign(), Fort.Name);
	const float GateEdge = FSolidStronghold::OpeningCm * 0.5f;
	const float FortNear = FortPole.Y - SolidNameSigns::BoardWidthCm(Fort.Name) * 0.5f;
	TestTrue(
		TEXT("the fortress board clears the gate by a step"),
		FortNear >= GateEdge + SolidNameSigns::StepCm - 0.1f);
	TestTrue(
		TEXT("the fortress pole stands on the grass face of the gate wall"),
		FMath::IsNearlyEqual(FortPole.X, -Fort.OuterDepthCm() * 0.5f, 0.1f));
	const FVector FortStand(-Fort.ExitReachCm(), 0.f, 0.f);
	TestTrue(
		TEXT("the fortress sign is clear of the captain's stand"),
		FVector::Dist(FortStand, FortPole) > SolidStronghold::DoorStandClearCm);
	const SolidNameSigns::FWorldPlace FortWorld = SolidNameSigns::WorldPlace(*Humble, nullptr);
	TestTrue(
		TEXT("a fortress sign faces the grass"),
		FMath::IsNearlyEqual(FRotator::NormalizeAxis(FortWorld.YawDeg), 0.f, 0.1f));

	const FSolidStronghold* Active = Cave.Get();
	TestTrue(TEXT("the sign stands for the company stronghold"), SolidNameSigns::StandsFor(Active, Active));
	TestFalse(TEXT("a stronghold the company has left has no sign"), SolidNameSigns::StandsFor(Active, Average.Get()));
	TestFalse(TEXT("no stronghold has no sign"), SolidNameSigns::StandsFor(nullptr, Active));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidNameSignActorTest,
	"SolidCore1.NameSign.Actor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidNameSignActorTest::RunTest(const FString& Parameters)
{
	ASolidNameSign* Sign = NewObject<ASolidNameSign>();
	TestNotNull(TEXT("sign"), Sign);
	if (!Sign)
	{
		return false;
	}
	Sign->SetStrongholdName(TEXT("High Keep"));
	TestEqual(TEXT("the board is the name"), Sign->GetLabel(), FString(TEXT("High Keep")));
	TestFalse(TEXT("the board is one line"), Sign->GetLabel().Contains(TEXT("\n")));
	TestFalse(TEXT("the board is not a welcome"), Sign->GetLabel().Contains(TEXT("Welcome")));
	UStaticMeshComponent* Pole = Sign->GetPoleMesh();
	UStaticMeshComponent* Board = Sign->GetBoardMesh();
	TestNotNull(TEXT("pole"), Pole);
	TestNotNull(TEXT("board"), Board);
	if (Pole)
	{
		TestEqual(
			TEXT("the pole does not block a pawn"),
			static_cast<int32>(Pole->GetCollisionResponseToChannel(ECC_Pawn)),
			static_cast<int32>(ECR_Ignore));
	}
	if (Board)
	{
		TestEqual(
			TEXT("the board does not block a pawn"),
			static_cast<int32>(Board->GetCollisionResponseToChannel(ECC_Pawn)),
			static_cast<int32>(ECR_Ignore));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidNameSignPageTest,
	"SolidCore1.NameSign.Page",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidNameSignPageTest::RunTest(const FString& Parameters)
{
	ASolidHUD* HUD = NewObject<ASolidHUD>();
	TestNotNull(TEXT("hud"), HUD);
	if (!HUD)
	{
		return false;
	}

	HUD->OpenStrongholdName(TEXT("The Cave"));
	TestTrue(TEXT("the page is open"), HUD->IsStrongholdNameOpen());
	TestEqual(TEXT("the title is the current name"), HUD->GetStrongholdNameTitle(), FString(TEXT("The Cave")));
	TestEqual(TEXT("the draft starts as that name"), HUD->GetStrongholdNameDraft(), FString(TEXT("The Cave")));
	HUD->TypeStrongholdName(EKeys::SpaceBar, false);
	HUD->TypeStrongholdName(EKeys::K, true);
	TestEqual(TEXT("the title stays the name at open"), HUD->GetStrongholdNameTitle(), FString(TEXT("The Cave")));
	TestEqual(TEXT("the draft takes the letters"), HUD->GetStrongholdNameDraft(), FString(TEXT("The Cave K")));

	FString Kept;
	TestTrue(TEXT("Enter keeps the trimmed name"), HUD->AcceptStrongholdName(Kept));
	TestEqual(TEXT("the kept name is The Cave K"), Kept, FString(TEXT("The Cave K")));
	TestFalse(TEXT("Enter closes the page"), HUD->IsStrongholdNameOpen());

	HUD->OpenStrongholdName(TEXT("The Cave"));
	HUD->TypeStrongholdName(EKeys::X, false);
	HUD->DismissStrongholdName();
	TestFalse(TEXT("Esc closes the page"), HUD->IsStrongholdNameOpen());
	TestEqual(TEXT("Esc does not replace the caller's name"), Kept, FString(TEXT("The Cave K")));

	HUD->OpenStrongholdName(TEXT("The Cave"));
	HUD->HandleMenuKey();
	TestFalse(TEXT("F10 closes the page"), HUD->IsStrongholdNameOpen());
	TestFalse(TEXT("F10 does not open the menu"), HUD->IsMainMenuOpen());

	HUD->OpenStrongholdName(TEXT("   "));
	FString Previous = TEXT("The Cave");
	TestFalse(TEXT("an empty trim is not kept"), HUD->AcceptStrongholdName(Previous));
	TestEqual(TEXT("the previous name stays"), Previous, FString(TEXT("The Cave")));
	TestFalse(TEXT("the empty trim still closes the page"), HUD->IsStrongholdNameOpen());

	HUD->OpenStrongholdName(TEXT("High Keep"));
	HUD->CloseMenuOverlay();
	TestFalse(TEXT("Esc from the overlay closes the page"), HUD->IsStrongholdNameOpen());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidNameSignMenuTest,
	"SolidCore1.NameSign.Menu",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidNameSignMenuTest::RunTest(const FString& Parameters)
{
	TArray<FSolidMainMenuEntry> Entries;
	SolidMainMenu::CollectEntries(Entries);
	TestTrue(TEXT("the menu has its rows"), Entries.Num() >= 2);
	if (Entries.Num() < 2)
	{
		return false;
	}
	TestEqual(
		TEXT("the last row is Exit Game"),
		static_cast<uint8>(Entries.Last().Item),
		static_cast<uint8>(ESolidMainMenuItem::ExitGame));
	TestEqual(TEXT("that row reads Exit Game"), Entries.Last().Label, FString(TEXT("Exit Game")));
	TestEqual(
		TEXT("the row before it is Credits"),
		static_cast<uint8>(Entries[Entries.Num() - 2].Item),
		static_cast<uint8>(ESolidMainMenuItem::Credits));
	TestEqual(TEXT("that row reads Credits"), Entries[Entries.Num() - 2].Label, FString(TEXT("Credits")));
	for (const FSolidMainMenuEntry& Entry : Entries)
	{
		TestFalse(TEXT("the menu has no name row"), Entry.Label.Contains(TEXT("Name")));
	}
	return true;
}

#endif
