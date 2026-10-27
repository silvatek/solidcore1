#include "SolidFortressInterior.h"
#include "SolidCore1.h"
#include "SolidMaterials.h"
#include "Strongholds/SolidStronghold.h"
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
	AddWall(Width * 0.5f, NorthY, Opening, Wall, (Storey + Height) * 0.5f, Height - Storey);
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

		if (!bWestCurtain)
		{
			AddBuilding(MinX + Wall * 0.5f, (MinY + MaxY) * 0.5f, Height * 0.5f, Wall, MaxY - MinY, Height);
		}
		if (!bEastCurtain)
		{
			AddBuilding(MaxX - Wall * 0.5f, (MinY + MaxY) * 0.5f, Height * 0.5f, Wall, MaxY - MinY, Height);
		}
		if (!bSouthCurtain)
		{
			AddBuilding((MinX + MaxX) * 0.5f, MinY + Wall * 0.5f, Height * 0.5f, MaxX - MinX, Wall, Height);
		}
		if (!bNorthCurtain)
		{
			const float DoorX = (MinX + MaxX) * 0.5f;
			const float DoorMin = DoorX - Opening * 0.5f;
			const float DoorMax = DoorX + Opening * 0.5f;
			const float WallY = MaxY - Wall * 0.5f;
			AddBuilding((MinX + DoorMin) * 0.5f, WallY, Height * 0.5f, DoorMin - MinX, Wall, Height);
			AddBuilding((DoorMax + MaxX) * 0.5f, WallY, Height * 0.5f, MaxX - DoorMax, Wall, Height);
			AddBuilding(DoorX, WallY, (Storey + Height) * 0.5f, Opening, Wall, Height - Storey);
		}

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
