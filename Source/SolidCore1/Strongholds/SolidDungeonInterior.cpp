#include "SolidDungeonInterior.h"
#include "SolidFurnishing.h"
#include "SolidStrongholdFurniture.h"
#include "SolidStrongholdGrid.h"
#include "SolidDungeonLevel.h"
#include "SolidCore1.h"
#include "SolidMaterials.h"
#include "SolidStrongholds.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Materials/MaterialInterface.h"

namespace
{
	void PlaceInteriorPart(
		UStaticMeshComponent* Part,
		UStaticMesh* Cube,
		AActor* Outer,
		const FVector& Location,
		const FVector& SizeCm,
		const FLinearColor& Color,
		const TCHAR* DebugName)
	{
		if (!Part || !Cube)
		{
			return;
		}

		constexpr float ShapeSize = 100.f;
		Part->SetStaticMesh(Cube);
		Part->SetRelativeLocation(Location);
		Part->SetRelativeRotation(FRotator::ZeroRotator);
		Part->SetRelativeScale3D(SizeCm / ShapeSize);
		if (UMaterialInterface* Material = SolidMaterials::CreateSolidColor(Outer, Color, DebugName, 0.95f))
		{
			Part->SetMaterial(0, Material);
		}
	}

	void DestroyOwnedFurnishings(AActor* Owner)
	{
		UWorld* World = Owner ? Owner->GetWorld() : nullptr;
		if (!World)
		{
			return;
		}

		TArray<ASolidFurnishing*> Owned;
		for (TActorIterator<ASolidFurnishing> It(World); It; ++It)
		{
			ASolidFurnishing* Piece = *It;
			if (Piece && Piece->GetOwner() == Owner && !Piece->IsActorBeingDestroyed())
			{
				Owned.Add(Piece);
			}
		}
		for (ASolidFurnishing* Piece : Owned)
		{
			Piece->Destroy();
		}
	}

	const SolidStrongholdGrid::FHex* FindFloorHex(
		const TArray<SolidStrongholdGrid::FHex>& Hexes,
		const int32 Col,
		const int32 Row)
	{
		for (const SolidStrongholdGrid::FHex& Hex : Hexes)
		{
			if (Hex.Col == Col && Hex.Row == Row)
			{
				return &Hex;
			}
		}
		return nullptr;
	}
}

ASolidDungeonInterior::ASolidDungeonInterior()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	FloorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FloorMesh"));
	CeilingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CeilingMesh"));
	WallBack = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WallBack"));
	WallLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WallLeft"));
	WallRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WallRight"));
	WallFront = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WallFront"));
	MouthMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MouthMesh"));
	RoomLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("RoomLight"));
	FloorGrid = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("FloorGrid"));

	UStaticMeshComponent* Parts[] = {
		FloorMesh.Get(), CeilingMesh.Get(), WallBack.Get(), WallLeft.Get(),
		WallRight.Get(), WallFront.Get(), MouthMesh.Get() };
	for (UStaticMeshComponent* Part : Parts)
	{
		Part->SetupAttachment(SceneRoot);
		Part->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Part->SetCollisionResponseToAllChannels(ECR_Block);
		Part->SetCastShadow(true);
	}

	FloorGrid->SetupAttachment(SceneRoot);
	FloorGrid->SetMobility(EComponentMobility::Movable);
	FloorGrid->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FloorGrid->SetCastShadow(false);

	RoomLight->SetupAttachment(SceneRoot);
	RoomLight->SetMobility(EComponentMobility::Movable);
	RoomLight->SetCastShadows(true);
	RoomLight->SetIntensity(6000.f);
	RoomLight->SetAttenuationRadius(2200.f);
	RoomLight->SetLightColor(FLinearColor(1.f, 0.82f, 0.55f));
}

void ASolidDungeonInterior::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	BuildVisuals();
}

void ASolidDungeonInterior::BeginPlay()
{
	Super::BeginPlay();
	BuildVisuals();
}

void ASolidDungeonInterior::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DestroyOwnedFurnishings(this);
	Super::EndPlay(EndPlayReason);
}

void ASolidDungeonInterior::BuildVisuals()
{
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!Cube)
	{
		UE_LOG(LogSolid, Error, TEXT("SolidDungeonInterior: missing Engine BasicShapes Cube."));
		return;
	}

	const float Depth = SolidDungeonLevel::RoomDepthCm;
	const float Width = SolidDungeonLevel::RoomWidthCm;
	const float Height = SolidDungeonLevel::RoomHeightCm;
	const float Shell = SolidDungeonLevel::ShellCm;
	const float SpanX = Depth + Shell * 2.f;
	const float SpanY = Width + Shell * 2.f;
	const FLinearColor Rock = SolidStrongholds::RockColor();

	PlaceInteriorPart(FloorMesh, Cube, this, FVector(Depth * 0.5f, 0.f, -Shell * 0.5f), FVector(SpanX, SpanY, Shell), Rock, TEXT("CaveFloor"));
	PlaceInteriorPart(CeilingMesh, Cube, this, FVector(Depth * 0.5f, 0.f, Height + Shell * 0.5f), FVector(SpanX, SpanY, Shell), Rock, TEXT("CaveCeiling"));
	PlaceInteriorPart(WallBack, Cube, this, FVector(Depth + Shell * 0.5f, 0.f, Height * 0.5f), FVector(Shell, SpanY, Height), Rock, TEXT("CaveWallBack"));
	PlaceInteriorPart(WallLeft, Cube, this, FVector(Depth * 0.5f, -(Width * 0.5f + Shell * 0.5f), Height * 0.5f), FVector(Depth, Shell, Height), Rock, TEXT("CaveWallLeft"));
	PlaceInteriorPart(WallRight, Cube, this, FVector(Depth * 0.5f, Width * 0.5f + Shell * 0.5f, Height * 0.5f), FVector(Depth, Shell, Height), Rock, TEXT("CaveWallRight"));
	PlaceInteriorPart(WallFront, Cube, this, FVector(-Shell * 0.5f, 0.f, Height * 0.5f), FVector(Shell, SpanY, Height), Rock, TEXT("CaveWallFront"));
	PlaceInteriorPart(
		MouthMesh,
		Cube,
		this,
		FVector(4.f, 0.f, SolidStrongholds::OpeningHeightCm * 0.5f),
		FVector(12.f, SolidStrongholds::OpeningWidthCm, SolidStrongholds::OpeningHeightCm),
		SolidStrongholds::MouthColor(),
		TEXT("CaveInteriorMouth"));

	if (RoomLight)
	{
		RoomLight->SetRelativeLocation(FVector(Depth * 0.5f, 0.f, Height * 0.72f));
	}

	if (FloorGrid)
	{
		FloorGrid->SetStaticMesh(Cube);
		FloorGrid->ClearInstances();
		if (UMaterialInterface* Material = SolidMaterials::CreateSolidColor(
			this, SolidStrongholdGrid::LineColor(), TEXT("DungeonHexGrid"), 0.45f))
		{
			FloorGrid->SetMaterial(0, Material);
		}

		TArray<SolidStrongholdGrid::FHex> Hexes;
		TArray<SolidStrongholdGrid::FSegment> Segments;
		SolidStrongholdGrid::CollectHexes(SolidStrongholdGrid::MeagreFloorBounds(), Hexes);
		SolidStrongholdGrid::CollectSegments(Hexes, Segments);

		const float ShapeSize = 100.f;
		const float LineZ = SolidStrongholdGrid::LineLiftCm + SolidStrongholdGrid::LineHeightCm * 0.5f;
		for (const SolidStrongholdGrid::FSegment& Segment : Segments)
		{
			const FVector2D Delta = Segment.B - Segment.A;
			const float Length = Delta.Size();
			if (Length < 1.f)
			{
				continue;
			}
			const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
			const FVector Mid((Segment.A.X + Segment.B.X) * 0.5f, (Segment.A.Y + Segment.B.Y) * 0.5f, LineZ);
			const FTransform Line(
				FRotator(0.f, Yaw, 0.f),
				Mid,
				FVector(
					Length / ShapeSize,
					SolidStrongholdGrid::LineThicknessCm / ShapeSize,
					SolidStrongholdGrid::LineHeightCm / ShapeSize));
			FloorGrid->AddInstance(Line);
		}
	}

	PlaceStarterFurnishings();
}

void ASolidDungeonInterior::PlaceStarterFurnishings()
{
	UWorld* World = GetWorld();
	// OnConstruction runs before the actor is initialized. Spawning then re-enters
	// construction, so the pieces are placed from the post-spawn BuildVisuals and BeginPlay.
	if (!World || !World->IsGameWorld() || World->bIsTearingDown || !IsActorInitialized())
	{
		return;
	}

	TArray<SolidStrongholdFurniture::FPlacement> Placements;
	SolidStrongholdFurniture::MeagreStarter(Placements);

	TArray<ASolidFurnishing*> Owned;
	for (TActorIterator<ASolidFurnishing> It(World); It; ++It)
	{
		ASolidFurnishing* Piece = *It;
		if (Piece && Piece->GetOwner() == this && !Piece->IsActorBeingDestroyed())
		{
			Owned.Add(Piece);
		}
	}
	if (Owned.Num() >= Placements.Num())
	{
		return;
	}
	for (ASolidFurnishing* Piece : Owned)
	{
		Piece->Destroy();
	}

	TArray<SolidStrongholdGrid::FHex> Hexes;
	SolidStrongholdGrid::CollectHexes(SolidStrongholdGrid::MeagreFloorBounds(), Hexes);

	int32 Placed = 0;
	for (const SolidStrongholdFurniture::FPlacement& Placement : Placements)
	{
		const SolidStrongholdGrid::FHex* Anchor = FindFloorHex(Hexes, Placement.Col, Placement.Row);
		const SolidStrongholdGrid::FHex* Span = FindFloorHex(Hexes, Placement.SpanCol, Placement.SpanRow);
		if (!Anchor || !Span)
		{
			UE_LOG(LogSolid, Error, TEXT("SolidDungeonInterior: starter furnishing hex (%d,%d) is not on the floor."),
				Placement.Col, Placement.Row);
			continue;
		}

		const FVector2D Mid = (Anchor->Center + Span->Center) * 0.5f;
		const FVector2D Delta = Span->Center - Anchor->Center;
		const float Yaw = Delta.IsNearlyZero() ? 0.f : FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
		const FTransform ActorXf = GetActorTransform();
		const FVector WorldPos = ActorXf.TransformPosition(FVector(Mid.X, Mid.Y, 0.f));
		const FRotator WorldRot = ActorXf.TransformRotation(FRotator(0.f, Yaw, 0.f).Quaternion()).Rotator();
		const FTransform SpawnXf(WorldRot, WorldPos);

		ASolidFurnishing* Piece = World->SpawnActorDeferred<ASolidFurnishing>(
			ASolidFurnishing::StaticClass(),
			SpawnXf,
			this,
			nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Piece)
		{
			UE_LOG(LogSolid, Error, TEXT("SolidDungeonInterior: failed to spawn a starter furnishing."));
			continue;
		}

		Piece->SetKind(Placement.Kind);
		Piece->FinishSpawning(SpawnXf);
		Piece->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
		++Placed;
	}

	UE_LOG(LogSolid, Warning, TEXT("Starter furnishings placed: %d."), Placed);
}

FVector ASolidDungeonInterior::GetCaptainSpawnWorld() const
{
	return GetActorTransform().TransformPosition(SolidDungeonLevel::CaptainSpawnLocal());
}

float ASolidDungeonInterior::GetCaptainYaw() const
{
	return SolidDungeonLevel::CaptainYawDeg;
}
