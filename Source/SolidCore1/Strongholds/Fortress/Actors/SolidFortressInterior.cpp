#include "SolidFortressInterior.h"
#include "SolidCore1.h"
#include "SolidMaterials.h"
#include "Strongholds/SolidStronghold.h"
#include "Strongholds/Furnishings/SolidArrangeSign.h"
#include "Strongholds/Furnishings/SolidStrongholdGrid.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

namespace SolidFortressInteriorPrivate
{
	constexpr float CubeSizeCm = 100.f;

	/** Plan +X is east and plan +Y is north. Actor -X is that north, the gate side. */
	FVector ActorCenter(const float Width, const float Depth, const float PlanX, const float PlanY, const float Z)
	{
		return FVector(Depth * 0.5f - PlanY, PlanX - Width * 0.5f, Z);
	}
}

ASolidFortressInterior::ASolidFortressInterior()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	FloorGrid = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("FloorGrid"));
	FloorGrid->SetupAttachment(SceneRoot);
	FloorGrid->SetMobility(EComponentMobility::Movable);
	FloorGrid->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FloorGrid->SetCastShadow(false);

	ArrangePlate = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ArrangePlate"));
	ArrangePlate->SetupAttachment(SceneRoot);
	ArrangePlate->SetMobility(EComponentMobility::Movable);
	ArrangePlate->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ArrangePlate->SetCollisionResponseToAllChannels(ECR_Ignore);
	ArrangePlate->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	ArrangePlate->SetCastShadow(false);

	ArrangeLetters = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ArrangeLetters"));
	ArrangeLetters->SetupAttachment(SceneRoot);
	ArrangeLetters->SetMobility(EComponentMobility::Movable);
	ArrangeLetters->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ArrangeLetters->SetCastShadow(false);
}

void ASolidFortressInterior::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	BuildVisuals();
}

void ASolidFortressInterior::BeginPlay()
{
	Super::BeginPlay();
	BuildVisuals();
}

void ASolidFortressInterior::DiscardParts()
{
	for (TObjectPtr<UStaticMeshComponent>& Part : Curtain)
	{
		if (UStaticMeshComponent* Mesh = Part.Get())
		{
			Mesh->DestroyComponent();
		}
	}
	Curtain.Reset();
	for (TObjectPtr<UStaticMeshComponent>& Part : Buildings)
	{
		if (UStaticMeshComponent* Mesh = Part.Get())
		{
			Mesh->DestroyComponent();
		}
	}
	Buildings.Reset();
	auto Drop = [](TObjectPtr<UStaticMeshComponent>& Part)
	{
		if (UStaticMeshComponent* Mesh = Part.Get())
		{
			Mesh->DestroyComponent();
			Part = nullptr;
		}
	};
	Drop(GateMesh);
	Drop(FloorMesh);
	for (TObjectPtr<UPointLightComponent>& Light : RoomLights)
	{
		if (UPointLightComponent* Point = Light.Get())
		{
			Point->DestroyComponent();
		}
	}
	RoomLights.Reset();
	if (FloorGrid)
	{
		FloorGrid->ClearInstances();
	}
}

UStaticMeshComponent* ASolidFortressInterior::AddPart(
	UStaticMesh* Mesh,
	const FVector& Location,
	const FVector& Scale,
	const FLinearColor& Color,
	const TCHAR* DebugName)
{
	if (!Mesh || !SceneRoot)
	{
		return nullptr;
	}

	UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(this, NAME_None, RF_Transient);
	if (!Part)
	{
		return nullptr;
	}

	Part->SetupAttachment(SceneRoot);
	Part->SetStaticMesh(Mesh);
	Part->SetRelativeLocation(Location);
	Part->SetRelativeRotation(FRotator::ZeroRotator);
	Part->SetRelativeScale3D(Scale);
	Part->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Part->SetCollisionResponseToAllChannels(ECR_Block);
	Part->SetCastShadow(true);
	if (UMaterialInterface* Material = SolidMaterials::CreateSolidColor(this, Color, DebugName, 0.94f))
	{
		Part->SetMaterial(0, Material);
	}
	if (GetWorld())
	{
		Part->RegisterComponent();
	}
	return Part;
}

void ASolidFortressInterior::BuildVisuals()
{
	DiscardParts();
	if (!Fortress)
	{
		return;
	}

	const float Width = Fortress->OuterWidthCm();
	const float Depth = Fortress->OuterDepthCm();
	const float Height = Fortress->CurtainHeightCm();
	const float Wall = FSolidStronghold::WallCm;
	const float Opening = FSolidStronghold::OpeningCm;
	const float Storey = FSolidStronghold::StoreyCm;
	if (Width <= Wall || Depth <= Wall || Height <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!Cube)
	{
		UE_LOG(LogSolid, Error, TEXT("SolidFortressInterior: missing Engine BasicShapes Cube."));
		return;
	}

	const FLinearColor Rock = SolidStrongholds::RockColor();

	auto AddWall = [&](
		const float PlanCenterX,
		const float PlanCenterY,
		const float PlanSizeX,
		const float PlanSizeY,
		const float CenterZ,
		const float SizeZ)
	{
		if (UStaticMeshComponent* Part = AddPart(
			Cube,
			SolidFortressInteriorPrivate::ActorCenter(Width, Depth, PlanCenterX, PlanCenterY, CenterZ),
			FVector(
				PlanSizeY / SolidFortressInteriorPrivate::CubeSizeCm,
				PlanSizeX / SolidFortressInteriorPrivate::CubeSizeCm,
				SizeZ / SolidFortressInteriorPrivate::CubeSizeCm),
			Rock,
			TEXT("FortressInteriorCurtain")))
		{
			Curtain.Add(Part);
		}
	};

	const float GateMinX = (Width - Opening) * 0.5f;
	const float GateMaxX = GateMinX + Opening;
	const float NorthY = Depth - Wall * 0.5f;
	AddWall(Width * 0.5f, Wall * 0.5f, Width, Wall, Height * 0.5f, Height);
	AddWall(Wall * 0.5f, Depth * 0.5f, Wall, Depth - 2.f * Wall, Height * 0.5f, Height);
	AddWall(Width - Wall * 0.5f, Depth * 0.5f, Wall, Depth - 2.f * Wall, Height * 0.5f, Height);
	AddWall(GateMinX * 0.5f, NorthY, GateMinX, Wall, Height * 0.5f, Height);
	AddWall((GateMaxX + Width) * 0.5f, NorthY, Width - GateMaxX, Wall, Height * 0.5f, Height);
	if (Height > Storey + KINDA_SMALL_NUMBER)
	{
		AddWall(Width * 0.5f, NorthY, Opening, Wall, (Storey + Height) * 0.5f, Height - Storey);
	}
	GateMesh = AddPart(
		Cube,
		SolidFortressInteriorPrivate::ActorCenter(Width, Depth, Width * 0.5f, NorthY, Storey * 0.5f),
		FVector(
			Wall / SolidFortressInteriorPrivate::CubeSizeCm,
			Opening / SolidFortressInteriorPrivate::CubeSizeCm,
			Storey / SolidFortressInteriorPrivate::CubeSizeCm),
		SolidStrongholds::MouthColor(),
		TEXT("FortressInteriorGate"));
	const float FloorThickness = 20.f;
	FloorMesh = AddPart(
		Cube,
		SolidFortressInteriorPrivate::ActorCenter(
			Width,
			Depth,
			Width * 0.5f,
			Depth * 0.5f,
			-FloorThickness * 0.5f),
		FVector(
			(Depth - 2.f * Wall) / SolidFortressInteriorPrivate::CubeSizeCm,
			(Width - 2.f * Wall) / SolidFortressInteriorPrivate::CubeSizeCm,
			FloorThickness / SolidFortressInteriorPrivate::CubeSizeCm),
		Rock,
		TEXT("FortressInteriorFloor"));

	auto AddBuilding = [&](
		const float PlanCenterX,
		const float PlanCenterY,
		const float CenterZ,
		const float PlanSizeX,
		const float PlanSizeY,
		const float SizeZ)
	{
		if (PlanSizeX <= KINDA_SMALL_NUMBER || PlanSizeY <= KINDA_SMALL_NUMBER || SizeZ <= KINDA_SMALL_NUMBER)
		{
			return;
		}
		if (UStaticMeshComponent* Part = AddPart(
			Cube,
			SolidFortressInteriorPrivate::ActorCenter(Width, Depth, PlanCenterX, PlanCenterY, CenterZ),
			FVector(
				PlanSizeY / SolidFortressInteriorPrivate::CubeSizeCm,
				PlanSizeX / SolidFortressInteriorPrivate::CubeSizeCm,
				SizeZ / SolidFortressInteriorPrivate::CubeSizeCm),
			Rock,
			TEXT("FortressInteriorBuilding")))
		{
			Buildings.Add(Part);
		}
	};

	for (const SolidStronghold::FSolidFortressBuilding& One : Fortress->Buildings)
	{
		float MinX = One.InnerMinCm.X - Wall;
		float MaxX = One.InnerMinCm.X + One.WidthCm() + Wall;
		float MinY = One.InnerMinCm.Y - Wall;
		float MaxY = One.InnerMinCm.Y + One.DepthCm() + Wall;
		const bool bWestCurtain = FMath::IsNearlyEqual(MinX, 0.f, 0.1f);
		const bool bEastCurtain = FMath::IsNearlyEqual(MaxX, Width, 0.1f);
		const bool bSouthCurtain = FMath::IsNearlyEqual(MinY, 0.f, 0.1f);
		const bool bNorthCurtain = FMath::IsNearlyEqual(MaxY, Depth, 0.1f);
		if (bWestCurtain)
		{
			MinX += Wall;
		}
		if (bEastCurtain)
		{
			MaxX -= Wall;
		}
		if (bSouthCurtain)
		{
			MinY += Wall;
		}
		if (bNorthCurtain)
		{
			MaxY -= Wall;
		}

		const float BuildingHeight = One.HeightCm(Fortress->ParapetCm);
		// A yard face is solid. The doorway is on the face the building faces.
		auto AddFace = [&](
			const bool bCurtain,
			const ESolidStrongholdOrientation Face,
			const float Fixed,
			const float SpanMin,
			const float SpanMax,
			const bool bSpanIsX)
		{
			if (bCurtain)
			{
				return;
			}
			const float MidZ = BuildingHeight * 0.5f;
			if (One.Facing != Face)
			{
				if (bSpanIsX)
				{
					AddBuilding((SpanMin + SpanMax) * 0.5f, Fixed, MidZ, SpanMax - SpanMin, Wall, BuildingHeight);
				}
				else
				{
					AddBuilding(Fixed, (SpanMin + SpanMax) * 0.5f, MidZ, Wall, SpanMax - SpanMin, BuildingHeight);
				}
				return;
			}
			// One floor centres the opening. A stair centres it on the landing.
			const float InnerStart = bSpanIsX ? One.InnerMinCm.X : One.InnerMinCm.Y;
			const float Door = One.DoorFromInnerCm() > 0.f
				? InnerStart + One.DoorFromInnerCm()
				: (SpanMin + SpanMax) * 0.5f;
			const float DoorMin = Door - Opening * 0.5f;
			const float DoorMax = Door + Opening * 0.5f;
			const float LintelZ = (Storey + BuildingHeight) * 0.5f;
			const float Lintel = BuildingHeight - Storey;
			if (bSpanIsX)
			{
				AddBuilding((SpanMin + DoorMin) * 0.5f, Fixed, MidZ, DoorMin - SpanMin, Wall, BuildingHeight);
				AddBuilding((DoorMax + SpanMax) * 0.5f, Fixed, MidZ, SpanMax - DoorMax, Wall, BuildingHeight);
				AddBuilding(Door, Fixed, LintelZ, Opening, Wall, Lintel);
			}
			else
			{
				AddBuilding(Fixed, (SpanMin + DoorMin) * 0.5f, MidZ, Wall, DoorMin - SpanMin, BuildingHeight);
				AddBuilding(Fixed, (DoorMax + SpanMax) * 0.5f, MidZ, Wall, SpanMax - DoorMax, BuildingHeight);
				AddBuilding(Fixed, Door, LintelZ, Wall, Opening, Lintel);
			}
		};
		AddFace(bWestCurtain, ESolidStrongholdOrientation::WestFace, MinX + Wall * 0.5f, MinY, MaxY, false);
		AddFace(bEastCurtain, ESolidStrongholdOrientation::EastFace, MaxX - Wall * 0.5f, MinY, MaxY, false);
		AddFace(bSouthCurtain, ESolidStrongholdOrientation::SouthFace, MinY + Wall * 0.5f, MinX, MaxX, true);
		AddFace(bNorthCurtain, ESolidStrongholdOrientation::NorthFace, MaxY - Wall * 0.5f, MinX, MaxX, true);

		const float InnerMinX = bWestCurtain ? MinX : MinX + Wall;
		const float InnerMaxX = bEastCurtain ? MaxX : MaxX - Wall;
		const float InnerMinY = bSouthCurtain ? MinY : MinY + Wall;
		const float InnerMaxY = bNorthCurtain ? MaxY : MaxY - Wall;
		AddBuilding(
			(InnerMinX + InnerMaxX) * 0.5f,
			(InnerMinY + InnerMaxY) * 0.5f,
			Storey + Wall * 0.5f,
			InnerMaxX - InnerMinX,
			InnerMaxY - InnerMinY,
			Wall);

		UPointLightComponent* Light = NewObject<UPointLightComponent>(this, NAME_None, RF_Transient);
		if (!Light)
		{
			continue;
		}
		Light->SetupAttachment(SceneRoot);
		Light->SetMobility(EComponentMobility::Movable);
		Light->SetCastShadows(true);
		Light->SetIntensity(4500.f);
		Light->SetAttenuationRadius(FMath::Max(InnerMaxX - InnerMinX, InnerMaxY - InnerMinY) * 1.4f);
		Light->SetLightColor(FLinearColor(1.f, 0.82f, 0.55f));
		Light->SetRelativeLocation(SolidFortressInteriorPrivate::ActorCenter(
			Width,
			Depth,
			(InnerMinX + InnerMaxX) * 0.5f,
			(InnerMinY + InnerMaxY) * 0.5f,
			Storey * 0.62f));
		if (GetWorld())
		{
			Light->RegisterComponent();
		}
		RoomLights.Add(Light);
	}

	if (!FloorGrid)
	{
		PlaceFurnishings();
		return;
	}
	FloorGrid->SetStaticMesh(Cube);
	FloorGrid->ClearInstances();
	if (UMaterialInterface* Material = SolidMaterials::CreateSolidColor(
		this, SolidStrongholdGrid::LineColor(), TEXT("FortressHexGrid"), 0.45f))
	{
		FloorGrid->SetMaterial(0, Material);
	}
	const float LineZ = SolidStrongholdGrid::LineLiftCm + SolidStrongholdGrid::LineHeightCm * 0.5f;
	for (const SolidStronghold::FSolidFortressBuilding& One : Fortress->Buildings)
	{
		TArray<SolidStrongholdGrid::FHex> Hexes;
		TArray<SolidStrongholdGrid::FSegment> Segments;
		One.CollectHexes(Hexes);
		SolidStrongholdGrid::CollectSegments(Hexes, Segments);
		for (const SolidStrongholdGrid::FSegment& Segment : Segments)
		{
			const FVector Start = SolidFortressInteriorPrivate::ActorCenter(Width, Depth, Segment.A.X, Segment.A.Y, LineZ);
			const FVector End = SolidFortressInteriorPrivate::ActorCenter(Width, Depth, Segment.B.X, Segment.B.Y, LineZ);
			const FVector Delta = End - Start;
			const float Length = Delta.Size2D();
			if (Length < 1.f)
			{
				continue;
			}
			FloorGrid->AddInstance(FTransform(
				FRotator(0.f, FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X)), 0.f),
				(Start + End) * 0.5f,
				FVector(
					Length / SolidFortressInteriorPrivate::CubeSizeCm,
					SolidStrongholdGrid::LineThicknessCm / SolidFortressInteriorPrivate::CubeSizeCm,
					SolidStrongholdGrid::LineHeightCm / SolidFortressInteriorPrivate::CubeSizeCm)));
		}
	}
	PlaceFurnishings();

	const FSolidArrangeSign Sign = FSolidArrangeSign::ForFortress(*Fortress);
	if (ArrangeLetters)
	{
		ArrangeLetters->SetStaticMesh(Cube);
		ArrangeLetters->ClearInstances();
		if (UMaterialInterface* Ink = SolidMaterials::CreateSolidColor(
			this, FSolidArrangeSign::TextColor(), TEXT("ArrangeLetters"), 0.9f))
		{
			ArrangeLetters->SetMaterial(0, Ink);
		}
		Sign.AppendLetters(ArrangeLetters, SolidFortressInteriorPrivate::CubeSizeCm);
	}
	if (ArrangePlate)
	{
		Sign.ApplyPlate(ArrangePlate, Cube, SolidFortressInteriorPrivate::CubeSizeCm);
	}
}

void ASolidFortressInterior::SetArrangeSignDone(const bool bDone)
{
	if (!ArrangeLetters || !Fortress)
	{
		return;
	}
	ArrangeLetters->ClearInstances();
	const FSolidArrangeSign Sign = FSolidArrangeSign::ForFortress(*Fortress);
	Sign.AppendLetters(ArrangeLetters, SolidFortressInteriorPrivate::CubeSizeCm, bDone);
}

void ASolidFortressInterior::SetFurnishings(const TArray<SolidStrongholdFurniture::FPlacedFurnishing>& Used)
{
	FurnishingsToPlace = Used;
	bUseFurnishings = true;
}

void ASolidFortressInterior::PlaceFurnishings()
{
	if (!Fortress || !bUseFurnishings)
	{
		return;
	}
	TArray<SolidStrongholdGrid::FHex> Hexes;
	Fortress->CollectGroundHexes(Fortress->Size, Hexes);
	SolidStrongholdFurniture::PlaceFurnishings(
		this,
		FurnishingsToPlace,
		Hexes,
		[this](const FVector2D& Mid, const float YawDeg)
		{
			return FTransform(
				FRotator(0.f, HexFacingInRoom(YawDeg), 0.f),
				HexPointInRoom(FVector(Mid.X, Mid.Y, 0.f)));
		});
}

FVector ASolidFortressInterior::GetCaptainSpawnWorld() const
{
	if (!Fortress)
	{
		return FVector::ZeroVector;
	}
	const float Width = Fortress->OuterWidthCm();
	const float Depth = Fortress->OuterDepthCm();
	const float InsideY = Depth - FSolidStronghold::WallCm - SolidStrongholdGrid::FlatToFlatCm;
	return SolidFortressInteriorPrivate::ActorCenter(Width, Depth, Width * 0.5f, InsideY, 0.f);
}

FRotator ASolidFortressInterior::GetCaptainView() const
{
	return FRotator::ZeroRotator;
}

int32 ASolidFortressInterior::ArrangeFloorCount() const
{
	return Fortress && Fortress->Supported(Fortress->Size) ? 1 : 0;
}

bool ASolidFortressInterior::CollectArrangeFloor(
	const int32 Floor,
	TArray<SolidStrongholdGrid::FHex>& OutCells) const
{
	if (!Fortress || Floor != 0)
	{
		OutCells.Reset();
		return false;
	}
	Fortress->CollectGroundHexes(Fortress->Size, OutCells);
	return OutCells.Num() > 0;
}

bool ASolidFortressInterior::ArrangeFloorZ(const int32 Floor, float& OutZ) const
{
	if (!Fortress || Floor != 0)
	{
		OutZ = 0.f;
		return false;
	}
	OutZ = 0.f;
	return true;
}

FVector ASolidFortressInterior::HexPointInRoom(const FVector& HexPoint) const
{
	if (!Fortress)
	{
		return HexPoint;
	}
	return SolidFortressInteriorPrivate::ActorCenter(
		Fortress->OuterWidthCm(),
		Fortress->OuterDepthCm(),
		HexPoint.X,
		HexPoint.Y,
		HexPoint.Z);
}

FVector ASolidFortressInterior::RoomPointOnHexGrid(const FVector& RoomPoint) const
{
	if (!Fortress)
	{
		return RoomPoint;
	}
	const float Width = Fortress->OuterWidthCm();
	const float Depth = Fortress->OuterDepthCm();
	return FVector(RoomPoint.Y + Width * 0.5f, Depth * 0.5f - RoomPoint.X, RoomPoint.Z);
}

FVector ASolidFortressInterior::HexDirectionInRoom(const FVector& HexDirection) const
{
	return FVector(-HexDirection.Y, HexDirection.X, HexDirection.Z);
}

FVector ASolidFortressInterior::RoomDirectionOnHexGrid(const FVector& RoomDirection) const
{
	return FVector(RoomDirection.Y, -RoomDirection.X, RoomDirection.Z);
}

float ASolidFortressInterior::HexFacingInRoom(const float HexFacingDeg) const
{
	return HexFacingDeg + 90.f;
}
